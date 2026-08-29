/**
  ******************************************************************************
  * @file    user_ble_uart.c
  * @brief   UART1 Beacon Mesh protocol, scheduler, relay and state machine
  ******************************************************************************
  */

#include "user_ble_uart.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "user_ble_mesh_logic.h"
#include "user_bsp_gpio.h"
#include "user_bsp_uart1.h"
#include "user_bsp_uart2.h"
#include "user_common.h"

#ifndef APP_BLE_DEBUG_LOG
#define APP_BLE_DEBUG_LOG        1U
#endif
#ifndef APP_VERBOSE_UART1_RX
#define APP_VERBOSE_UART1_RX     1U
#endif

#define APP_BLE_NETWORK_ID           0x1234U
#define APP_UART_CMD_MODULE_READY    0x20U
#define APP_UART_CMD_MAC             0x71U
#define APP_UART_CMD_BEACON_RX       0x91U
#define APP_UART_CMD_BEACON_TX       0x92U
#define APP_BEACON_CMD_LEADER_ADV    0x80U
#define APP_BEACON_CMD_SYNC_TICK     0x81U
#define APP_BEACON_CMD_LEADER_RESIGN 0x82U
#define APP_BEACON_CMD_REMOTE_RELAY  0x83U
#define APP_BEACON_LENGTH             25U
#define APP_REMOTE_LENGTH             16U
#define APP_UART_HEAD_MODULE_H        0xAAU
#define APP_UART_HEAD_MODULE_L        0x55U
#define APP_UART_HEAD_MCU_H           0x55U
#define APP_UART_HEAD_MCU_L           0xAAU
#define APP_UART_TAIL                 0xFEU

#define APP_MESH_TX_QUEUE_CAPACITY    8U
#define APP_MESH_RELAY_CAPACITY       4U
#define APP_MESH_DEDUP_CAPACITY       32U
#define APP_MESH_DEDUP_TTL_MS         1000UL
#define APP_MESH_ADV_MS               1000UL
#define APP_MESH_SYNC_MS              1000UL
#define APP_MESH_SYNC_PHASE_MS        500UL
#define APP_MESH_UART_MIN_MS          200UL
#define APP_MESH_BLE_HOLD_MS          400UL
#define APP_MESH_LEADER_TIMEOUT_MS    3000UL
#define APP_MESH_CANDIDATE_BASE_MS    300UL
#define APP_MESH_CANDIDATE_WINDOW_MS  200UL
#define APP_MESH_RELAY_MIN_MS         20UL
#define APP_MESH_RELAY_MAX_MS         80UL
#define APP_MESH_POWER_LED_MS         3000UL
#define APP_MESH_HEARTBEAT_LED_MS     100UL
#define APP_MESH_QUERY_RETRY_MS       500UL

#define APP_MESH_MS_TO_TICKS(ms) \
	((uint32_t)(((uint64_t)(ms) * 1000ULL) / (uint64_t)USER_WOS_TICK_US))

#define APP_TICKS_500MS              APP_MESH_MS_TO_TICKS(500UL)
#define APP_TICKS_400MS              APP_MESH_MS_TO_TICKS(APP_MESH_BLE_HOLD_MS)
#define APP_TICKS_ADV_PERIOD         APP_MESH_MS_TO_TICKS(APP_MESH_ADV_MS)
#define APP_TICKS_SYNC_PERIOD        APP_MESH_MS_TO_TICKS(APP_MESH_SYNC_MS)
#define APP_TICKS_3000MS             APP_MESH_MS_TO_TICKS(APP_MESH_LEADER_TIMEOUT_MS)
#define APP_TICKS_MODULE_QUERY_RETRY APP_MESH_MS_TO_TICKS(APP_MESH_QUERY_RETRY_MS)
#define APP_TICKS_RELAY_MIN          APP_MESH_MS_TO_TICKS(APP_MESH_RELAY_MIN_MS)
#define APP_TICKS_RELAY_RANGE        APP_MESH_MS_TO_TICKS(APP_MESH_RELAY_MAX_MS - APP_MESH_RELAY_MIN_MS + 1UL)

static const uint8_t g_remote_cid[4] = {0x03U, 0x09U, 0x4CU, 0x5AU};

typedef enum
{
	MESH_ROLE_INIT = 0U,
	MESH_ROLE_FOLLOWER,
	MESH_ROLE_CANDIDATE,
	MESH_ROLE_LEADER
} mesh_role_t;

typedef enum
{
	MESH_TX_RESIGN = 0U,
	MESH_TX_ADV,
	MESH_TX_RELAY,
	MESH_TX_SYNC
} mesh_tx_type_t;

typedef enum
{
	MESH_TX_PENDING = 0U,
	MESH_TX_SENT,
	MESH_TX_CANCELLED,
	MESH_TX_EXPIRED
} mesh_tx_state_t;

typedef enum
{
	MESH_RELAY_BEACON = 0U,
	MESH_RELAY_REMOTE_83
} mesh_relay_kind_t;

typedef app_mesh_packet_key_t mesh_beacon_key_t;

typedef struct
{
	mesh_beacon_key_t key;
	uint8_t valid;
} beacon_dedup_entry_t;

typedef struct
{
	mesh_tx_type_t type;
	uint8_t priority;
	uint8_t active;
	uint8_t has_remote_key;
	mesh_tx_state_t state;
	uint8_t beacon[APP_BEACON_LENGTH];
	mesh_beacon_key_t beacon_key;
	app_mesh_remote_key_t remote_key;
	uint32_t created_at;
	uint32_t due_at;
	uint32_t expire_at;
	uint32_t last_wait_log;
} mesh_tx_item_t;

typedef struct
{
	uint8_t active;
	mesh_relay_kind_t kind;
	uint32_t scheduled_at;
	uint32_t due_at;
	uint32_t expire_at;
	mesh_beacon_key_t beacon_key;
	app_mesh_remote_key_t remote_key;
	uint8_t beacon[APP_BEACON_LENGTH];
} relay_pending_t;

typedef struct
{
	mesh_role_t role;
	uint8_t module_ready;
	uint8_t mac_ready;
	uint8_t mac_query_pending;
	uint8_t election_id[6];
	uint8_t current_leader[6];
	uint8_t has_current_leader;
	uint8_t seq;
	uint32_t last_leader_seen;
	uint32_t candidate_started;
	uint32_t candidate_delay;
	uint32_t next_adv_due;
	uint32_t next_sync_due;
	uint32_t last_uart_tx;
	uint32_t beacon_hold_until;
	uint32_t last_mac_query;
	uint32_t handshake_started;
	uint32_t led_start;
	uint32_t heartbeat_led_start;
	uint8_t heartbeat_led_active;
	uint8_t led_output;
	uint8_t sync_dynamic;
	uint8_t sync_payload[19];
	mesh_tx_item_t tx_queue[APP_MESH_TX_QUEUE_CAPACITY];
	relay_pending_t relay[APP_MESH_RELAY_CAPACITY];
	beacon_dedup_entry_t beacon_dedup[APP_MESH_DEDUP_CAPACITY];
	uint32_t beacon_dedup_expiry[APP_MESH_DEDUP_CAPACITY];
	uint8_t beacon_dedup_count[APP_MESH_DEDUP_CAPACITY];
	app_mesh_remote_key_t remote_dedup[APP_MESH_DEDUP_CAPACITY];
	uint32_t remote_dedup_expiry[APP_MESH_DEDUP_CAPACITY];
	uint8_t remote_dedup_count[APP_MESH_DEDUP_CAPACITY];
	uint8_t remote_dedup_valid[APP_MESH_DEDUP_CAPACITY];
	uint8_t beacon_dedup_cursor;
	uint8_t remote_dedup_cursor;
} mesh_runtime_t;

static mesh_runtime_t g_mesh;

#if APP_MESH_TRACE_LOG
static uint32_t APP_MeshNowMs(void)
{
	return (uint32_t)(((uint64_t)wos * (uint64_t)USER_WOS_TICK_US) / 1000ULL);
}
#endif

/*
 * Mesh timestamps are stored as uint32_t wos ticks.  Keep subtraction
 * wrap-safe, but reject a genuinely stale sample instead of allowing an
 * unsigned underflow to become a huge diagnostic delay.
 */
static uint32_t APP_MeshTicksToMs(uint32_t ticks)
{
	return (uint32_t)(((uint64_t)ticks * (uint64_t)USER_WOS_TICK_US) / 1000ULL);
}

static uint32_t APP_MeshNonNegativeDeltaMs(uint32_t now, uint32_t since)
{
	int32_t delta = (int32_t)(now - since);

	if (delta < 0) return 0U;
	return APP_MeshTicksToMs((uint32_t)delta);
}

static uint8_t APP_MeshElapsed(uint32_t now, uint32_t since, uint32_t period)
{
	return ((uint32_t)(now - since) >= period) ? 1U : 0U;
}

static uint8_t APP_MeshTimeReached(uint32_t now, uint32_t deadline)
{
	return ((int32_t)(now - deadline) >= 0) ? 1U : 0U;
}

static uint32_t APP_MeshCandidateDelayTicks(void)
{
	uint32_t delay_ms = APP_MESH_CANDIDATE_BASE_MS +
	                    ((uint32_t)g_mesh.election_id[5] % APP_MESH_CANDIDATE_WINDOW_MS);
	return APP_MESH_MS_TO_TICKS(delay_ms);
}

#if APP_MESH_TRACE_LOG
#define APP_MESH_TRACE(module, ...) do { \
	printf("T=%06lu [" module "] ", (unsigned long)APP_MeshNowMs()); \
	printf(__VA_ARGS__); \
	printf("\r\n"); \
} while (0)
#else
#define APP_MESH_TRACE(module, ...) do { \
	if (0) printf(__VA_ARGS__); \
	(void)(module); \
} while (0)
#endif

static const char *APP_MeshRoleName(mesh_role_t role)
{
	switch (role)
	{
	case MESH_ROLE_INIT:      return "INIT";
	case MESH_ROLE_FOLLOWER:  return "FOLLOWER";
	case MESH_ROLE_CANDIDATE: return "CANDIDATE";
	case MESH_ROLE_LEADER:    return "LEADER";
	default:                  return "UNKNOWN";
	}
}

static const char *APP_MeshTxName(mesh_tx_type_t type)
{
	switch (type)
	{
	case MESH_TX_RESIGN: return "LEADER_RESIGN";
	case MESH_TX_ADV:    return "LEADER_ADV";
	case MESH_TX_RELAY:  return "RELAY";
	case MESH_TX_SYNC:   return "SYNC_TICK";
	default:             return "UNKNOWN";
	}
}

static const char *APP_MeshCmdName(uint8_t cmd)
{
	switch (cmd)
	{
	case APP_BEACON_CMD_LEADER_ADV:    return "LEADER_ADV";
	case APP_BEACON_CMD_SYNC_TICK:     return "SYNC_TICK";
	case APP_BEACON_CMD_LEADER_RESIGN: return "LEADER_RESIGN";
	case APP_BEACON_CMD_REMOTE_RELAY:  return "REMOTE_CONTROL_RELAY";
	default:                            return "UNKNOWN";
	}
}

static const char *APP_MeshRemoteTypeName(uint8_t cmd_type)
{
	switch (cmd_type)
	{
	case 0x00U: return "SHORT";
	case 0x01U: return "LONG";
	case 0x02U: return "RELEASE";
	case 0xAAU: return "TEST";
	default:    return "UNKNOWN";
	}
}

static uint8_t APP_MeshIsZeroId(const uint8_t id[6])
{
	uint8_t i;

	for (i = 0U; i < 6U; i++)
	{
		if (id[i] != 0U) return 0U;
	}
	return 1U;
}

static int8_t APP_MeshCompareId(const uint8_t left[6], const uint8_t right[6])
{
	uint8_t i;

	for (i = 0U; i < 6U; i++)
	{
		if (left[i] < right[i]) return -1;
		if (left[i] > right[i]) return 1;
	}
	return 0;
}

static uint8_t APP_MeshBeaconKeyEqual(const mesh_beacon_key_t *left,
	                                  const mesh_beacon_key_t *right)
{
	return APP_MeshLogicPacketKeyEqual(left, right);
}

static uint32_t APP_MeshPseudoRandom(uint32_t now, uint8_t salt)
{
	uint32_t value = now ^ ((uint32_t)salt * 2654435761UL);
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	return value;
}

static uint8_t APP_MeshNextSeq(void)
{
	g_mesh.seq++;
	if (g_mesh.seq == 0U) g_mesh.seq = 1U;
	return g_mesh.seq;
}

static void APP_MeshLedSet(uint8_t on)
{
	on = (on != 0U) ? 1U : 0U;
	if (g_mesh.led_output == on) return;
	g_mesh.led_output = on;
	if (on != 0U) LEDB_L;
	else LEDB_H;
}

static void APP_MeshLedUpdate(uint32_t now)
{
	uint8_t led_on = 0U;
	uint32_t elapsed = (uint32_t)(now - g_mesh.led_start);

	if (elapsed < APP_MESH_MS_TO_TICKS(APP_MESH_POWER_LED_MS))
	{
		led_on = (uint8_t)((elapsed / APP_TICKS_500MS) & 1UL);
	}
	else if (g_mesh.role == MESH_ROLE_LEADER)
	{
		led_on = 1U;
	}
	else if ((g_mesh.role == MESH_ROLE_FOLLOWER) &&
	         (g_mesh.heartbeat_led_active != 0U))
	{
		if (APP_MeshElapsed(now, g_mesh.heartbeat_led_start,
		                   APP_MESH_MS_TO_TICKS(APP_MESH_HEARTBEAT_LED_MS)) != 0U)
		{
			g_mesh.heartbeat_led_active = 0U;
		}
		else
		{
			led_on = 1U;
		}
	}
	APP_MeshLedSet(led_on);
}

static void APP_MeshSetRole(mesh_role_t role, const char *reason)
{
	mesh_role_t old_role = g_mesh.role;
	uint8_t i;
	(void)reason;

	g_mesh.role = role;
	if (role != MESH_ROLE_FOLLOWER) g_mesh.heartbeat_led_active = 0U;
	if (role != MESH_ROLE_LEADER)
	{
		for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
		{
			if ((g_mesh.tx_queue[i].active != 0U) &&
			    ((g_mesh.tx_queue[i].type == MESH_TX_ADV) ||
			     (g_mesh.tx_queue[i].type == MESH_TX_SYNC)))
			{
				APP_MESH_TRACE("TXQ", "CANCEL type=%s reason=ROLE_NOT_LEADER",
				               APP_MeshTxName(g_mesh.tx_queue[i].type));
				g_mesh.tx_queue[i].state = MESH_TX_CANCELLED;
				g_mesh.tx_queue[i].active = 0U;
			}
		}
	}
	if (old_role != role)
	{
		APP_MESH_TRACE("MESH", "ROLE from=%s to=%s reason=%s leader=%02X%02X%02X%02X%02X%02X",
		               APP_MeshRoleName(old_role), APP_MeshRoleName(role), reason,
		               g_mesh.current_leader[0], g_mesh.current_leader[1],
		               g_mesh.current_leader[2], g_mesh.current_leader[3],
		               g_mesh.current_leader[4], g_mesh.current_leader[5]);
	}
}

static uint8_t APP_MeshQueueDepth(void)
{
	uint8_t i;
	uint8_t depth = 0U;

	for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
		if (g_mesh.tx_queue[i].active != 0U) depth++;
	return depth;
}

static void APP_MeshQueueDrop(mesh_tx_item_t *item, mesh_tx_state_t state,
	                          const char *reason)
{
	item->state = state;
	(void)reason;
	APP_MESH_TRACE("TXQ", "%s type=%s reason=%s depth=%u",
	               (state == MESH_TX_EXPIRED) ? "EXPIRE" : "DROP",
	               APP_MeshTxName(item->type), reason,
	               (unsigned int)APP_MeshQueueDepth());
	item->active = 0U;
}

static int8_t APP_MeshFindFreeTxSlot(uint32_t now)
{
	uint8_t i;

	for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
	{
		if (g_mesh.tx_queue[i].active == 0U) return (int8_t)i;
		if ((g_mesh.tx_queue[i].expire_at != 0U) &&
		    APP_MeshTimeReached(now, g_mesh.tx_queue[i].expire_at) != 0U)
		{
			APP_MeshQueueDrop(&g_mesh.tx_queue[i], MESH_TX_EXPIRED, "QUEUE_EXPIRED");
			return (int8_t)i;
		}
	}
	return -1;
}

static uint8_t APP_MeshQueueBeacon(mesh_tx_type_t type, uint8_t priority,
	                               const uint8_t beacon[APP_BEACON_LENGTH],
	                               uint32_t due_at, uint32_t expire_at,
	                               const mesh_beacon_key_t *key,
	                               const app_mesh_remote_key_t *remote_key)
{
	int8_t slot;
	uint32_t now = wos;
	mesh_tx_item_t *item;

	if (type == MESH_TX_SYNC)
	{
		slot = -1;
		for (uint8_t i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
		{
			if ((g_mesh.tx_queue[i].active != 0U) &&
			    (g_mesh.tx_queue[i].type == MESH_TX_SYNC))
			{
				slot = (int8_t)i;
				break;
			}
		}
		if (slot >= 0)
		{
			item = &g_mesh.tx_queue[(uint8_t)slot];
			item->state = MESH_TX_CANCELLED;
			item->active = 1U;
			item->has_remote_key = 0U;
			memcpy(item->beacon, beacon, APP_BEACON_LENGTH);
			item->beacon_key = *key;
			item->priority = priority;
			item->state = MESH_TX_PENDING;
			item->created_at = now;
			item->due_at = due_at;
			item->expire_at = expire_at;
			item->last_wait_log = 0U;
			APP_MESH_TRACE("TXQ", "DROP type=SYNC_TICK reason=SUPERSEDED");
			APP_MESH_TRACE("TXQ", "ENQUEUE type=SYNC_TICK priority=%u due_ms=%lu depth=%u",
			               (unsigned int)priority,
			               (unsigned long)((uint64_t)due_at * USER_WOS_TICK_US / 1000ULL),
			               (unsigned int)APP_MeshQueueDepth());
			return 1U;
		}
	}

	slot = APP_MeshFindFreeTxSlot(now);
	if (slot < 0)
	{
		APP_MESH_TRACE("TXQ", "DROP type=%s reason=QUEUE_FULL depth=%u",
		               APP_MeshTxName(type), (unsigned int)APP_MeshQueueDepth());
		return 0U;
	}
	item = &g_mesh.tx_queue[(uint8_t)slot];
	memset(item, 0, sizeof(*item));
	item->active = 1U;
	item->state = MESH_TX_PENDING;
	item->type = type;
	item->priority = priority;
	item->created_at = now;
	item->due_at = due_at;
	item->expire_at = expire_at;
	memcpy(item->beacon, beacon, APP_BEACON_LENGTH);
	item->beacon_key = *key;
	if (remote_key != (const app_mesh_remote_key_t *)0)
	{
		item->has_remote_key = 1U;
		item->remote_key = *remote_key;
	}
	APP_MESH_TRACE("TXQ", "ENQUEUE type=%s priority=%u due_ms=%lu depth=%u",
	               APP_MeshTxName(type), (unsigned int)priority,
	               (unsigned long)((uint64_t)due_at * USER_WOS_TICK_US / 1000ULL),
	               (unsigned int)APP_MeshQueueDepth());
	return 1U;
}

static void APP_MeshCancelRelayBeacon(const mesh_beacon_key_t *key,
                                  const char *reason)
{
	uint8_t i;
	(void)reason;

	for (i = 0U; i < APP_MESH_RELAY_CAPACITY; i++)
	{
		if ((g_mesh.relay[i].active != 0U) &&
		    (g_mesh.relay[i].kind == MESH_RELAY_BEACON) &&
		    APP_MeshBeaconKeyEqual(&g_mesh.relay[i].beacon_key, key))
		{
			g_mesh.relay[i].active = 0U;
			APP_MESH_TRACE("RELAY", "CANCEL key=%04X:%u:%02X reason=%s",
			               (unsigned int)key->network_id, (unsigned int)key->seq,
			               (unsigned int)key->cmd, reason);
		}
	}
	for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
	{
		if ((g_mesh.tx_queue[i].active != 0U) &&
		    (g_mesh.tx_queue[i].type == MESH_TX_RELAY) &&
		    (g_mesh.tx_queue[i].has_remote_key == 0U) &&
		    APP_MeshBeaconKeyEqual(&g_mesh.tx_queue[i].beacon_key, key))
		{
			g_mesh.tx_queue[i].state = MESH_TX_CANCELLED;
			g_mesh.tx_queue[i].active = 0U;
			APP_MESH_TRACE("TXQ", "CANCEL type=RELAY key=%04X:%u:%02X reason=%s",
			               (unsigned int)key->network_id, (unsigned int)key->seq,
			               (unsigned int)key->cmd, reason);
		}
	}
}

static void APP_MeshCancelRelayRemote(const app_mesh_remote_key_t *key,
	                                  const char *reason)
{
	uint8_t i;
	(void)reason;

	for (i = 0U; i < APP_MESH_RELAY_CAPACITY; i++)
	{
		if ((g_mesh.relay[i].active != 0U) &&
		    (g_mesh.relay[i].kind == MESH_RELAY_REMOTE_83) &&
		    APP_MeshLogicRemoteKeyEqual(&g_mesh.relay[i].remote_key, key))
		{
			g_mesh.relay[i].active = 0U;
			APP_MESH_TRACE("RELAY", "CANCEL key=%04X:%u:%02X:%02X:%02X reason=%s",
			               (unsigned int)key->address, (unsigned int)key->count,
			               (unsigned int)key->cmd, (unsigned int)key->cmd_type,
			               (unsigned int)key->para, reason);
		}
	}
	for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
	{
		if ((g_mesh.tx_queue[i].active != 0U) &&
		    (g_mesh.tx_queue[i].type == MESH_TX_RELAY) &&
		    (g_mesh.tx_queue[i].has_remote_key != 0U) &&
		    APP_MeshLogicRemoteKeyEqual(&g_mesh.tx_queue[i].remote_key, key))
		{
			g_mesh.tx_queue[i].state = MESH_TX_CANCELLED;
			g_mesh.tx_queue[i].active = 0U;
			APP_MESH_TRACE("TXQ", "CANCEL type=RELAY key=%04X:%u:%02X:%02X:%02X reason=%s",
			               (unsigned int)key->address, (unsigned int)key->count,
			               (unsigned int)key->cmd, (unsigned int)key->cmd_type,
			               (unsigned int)key->para, reason);
		}
	}
}

static uint8_t APP_MeshScheduleBeaconRelay(const uint8_t beacon[APP_BEACON_LENGTH],
	                                       const mesh_beacon_key_t *key)
{
	uint8_t i;
	uint32_t now = wos;
	uint32_t delay;

	for (i = 0U; i < APP_MESH_RELAY_CAPACITY; i++)
	{
		if (g_mesh.relay[i].active == 0U)
		{
			memset(&g_mesh.relay[i], 0, sizeof(g_mesh.relay[i]));
			g_mesh.relay[i].active = 1U;
			g_mesh.relay[i].kind = MESH_RELAY_BEACON;
			g_mesh.relay[i].scheduled_at = now;
			delay = APP_TICKS_RELAY_MIN +
			        (APP_MeshPseudoRandom(now, (uint8_t)(key->seq ^ key->cmd ^ g_mesh.election_id[5])) % APP_TICKS_RELAY_RANGE);
			g_mesh.relay[i].due_at = now + delay;
			g_mesh.relay[i].expire_at = now + APP_TICKS_400MS + APP_TICKS_RELAY_MIN;
			g_mesh.relay[i].beacon_key = *key;
			memcpy(g_mesh.relay[i].beacon, beacon, APP_BEACON_LENGTH);
			APP_MESH_TRACE("RELAY", "SCHEDULE kind=BEACON key=%04X:%u:%02X delay_ms=%lu due_ms=%lu",
			               (unsigned int)key->network_id, (unsigned int)key->seq,
			               (unsigned int)key->cmd,
			               (unsigned long)((uint64_t)delay * USER_WOS_TICK_US / 1000ULL),
			               (unsigned long)((uint64_t)g_mesh.relay[i].due_at * USER_WOS_TICK_US / 1000ULL));
			return 1U;
		}
	}
	APP_MESH_TRACE("RELAY", "SUPPRESS key=%04X:%u:%02X reason=RELAY_QUEUE_FULL",
	               (unsigned int)key->network_id, (unsigned int)key->seq,
	               (unsigned int)key->cmd);
	return 0U;
}

static uint8_t APP_MeshScheduleRemoteRelay(const app_mesh_remote_key_t *remote,
	                                       uint32_t *delay_out)
{
	uint8_t i;
	uint32_t now = wos;
	uint32_t delay;

	for (i = 0U; i < APP_MESH_RELAY_CAPACITY; i++)
	{
		if (g_mesh.relay[i].active == 0U)
		{
			memset(&g_mesh.relay[i], 0, sizeof(g_mesh.relay[i]));
			g_mesh.relay[i].active = 1U;
			g_mesh.relay[i].kind = MESH_RELAY_REMOTE_83;
			g_mesh.relay[i].scheduled_at = now;
			delay = APP_TICKS_RELAY_MIN +
			        (APP_MeshPseudoRandom(now, (uint8_t)(remote->count ^ remote->cmd ^ g_mesh.election_id[5])) % APP_TICKS_RELAY_RANGE);
			g_mesh.relay[i].due_at = now + delay;
			g_mesh.relay[i].expire_at = now + APP_TICKS_400MS + APP_TICKS_RELAY_MIN;
			g_mesh.relay[i].remote_key = *remote;
			if (delay_out != (uint32_t *)0) *delay_out = delay;
			APP_MESH_TRACE("RELAY", "SCHEDULE kind=REMOTE_83 key=%04X:%u:%02X:%02X:%02X delay_ms=%lu",
			               (unsigned int)remote->address, (unsigned int)remote->count,
			               (unsigned int)remote->cmd, (unsigned int)remote->cmd_type,
			               (unsigned int)remote->para,
			               (unsigned long)((uint64_t)delay * USER_WOS_TICK_US / 1000ULL));
			return 1U;
		}
	}
	APP_MESH_TRACE("RELAY", "SUPPRESS key=%04X:%u:%02X:%02X:%02X reason=RELAY_QUEUE_FULL",
	               (unsigned int)remote->address, (unsigned int)remote->count,
	               (unsigned int)remote->cmd, (unsigned int)remote->cmd_type,
	               (unsigned int)remote->para);
	return 0U;
}

static void APP_MeshProcessRelay(uint32_t now)
{
	uint8_t i;
	uint8_t encoded[APP_BEACON_LENGTH];
	mesh_beacon_key_t key;
	app_mesh_beacon_t beacon;

	/* UART2 trace output in the caller may have made its timestamp stale. */
	now = wos;

	for (i = 0U; i < APP_MESH_RELAY_CAPACITY; i++)
	{
		if (g_mesh.relay[i].active == 0U) continue;
		if (APP_MeshTimeReached(now, g_mesh.relay[i].expire_at) != 0U)
		{
			APP_MESH_TRACE("RELAY", "CANCEL reason=RELAY_QUEUE_TIMEOUT");
			g_mesh.relay[i].active = 0U;
			continue;
		}
		if (APP_MeshTimeReached(now, g_mesh.relay[i].due_at) == 0U) continue;

		if (g_mesh.relay[i].kind == MESH_RELAY_BEACON)
		{
			if (APP_MeshLogicBuildBeaconRelay(g_mesh.relay[i].beacon, encoded) == 0U)
			{
				APP_MESH_TRACE("RELAY", "SUPPRESS reason=INVALID_SNAPSHOT");
				g_mesh.relay[i].active = 0U;
				continue;
			}
			(void)APP_MeshLogicDecodeBeacon(encoded, &beacon);
			key.network_id = beacon.network_id;
			key.seq = beacon.seq;
			key.cmd = beacon.cmd;
			if (APP_MeshQueueBeacon(MESH_TX_RELAY, 3U, encoded, now,
			                        now + APP_TICKS_400MS, &key,
			                        (const app_mesh_remote_key_t *)0) == 0U)
			{
				APP_MESH_TRACE("RELAY", "SUPPRESS kind=BEACON key=%04X:%u:%02X reason=TX_QUEUE_FULL",
				               (unsigned int)key.network_id, (unsigned int)key.seq,
				               (unsigned int)key.cmd);
			}
			APP_MESH_TRACE("RELAY", "READY kind=BEACON key=%04X:%u:%02X flags=0x%02X",
			               (unsigned int)key.network_id, (unsigned int)key.seq,
			               (unsigned int)key.cmd, (unsigned int)beacon.flags);
		}
		else
		{
			APP_MeshLogicBuildRemoteRelay(&g_mesh.relay[i].remote_key, encoded);
			key.network_id = g_mesh.relay[i].remote_key.address;
			key.seq = g_mesh.relay[i].remote_key.count;
			key.cmd = APP_BEACON_CMD_REMOTE_RELAY;
			if (APP_MeshQueueBeacon(MESH_TX_RELAY, 3U, encoded, now,
			                        now + APP_TICKS_400MS, &key,
			                        &g_mesh.relay[i].remote_key) == 0U)
			{
				APP_MESH_TRACE("RELAY", "SUPPRESS kind=REMOTE_83 key=%04X:%u:%02X reason=TX_QUEUE_FULL",
				               (unsigned int)key.network_id, (unsigned int)key.seq,
				               (unsigned int)key.cmd);
			}
			APP_MESH_TRACE("RELAY", "READY kind=REMOTE_83 key=%04X:%u:%02X flags=0x03",
			               (unsigned int)key.network_id, (unsigned int)key.seq,
			               (unsigned int)key.cmd);
		}
		g_mesh.relay[i].active = 0U;
	}
}

static uint8_t APP_MeshBeaconDedup(const mesh_beacon_key_t *key, uint32_t now,
	                               const char *origin)
{
	uint8_t i;
	uint8_t slot = g_mesh.beacon_dedup_cursor;

	for (i = 0U; i < APP_MESH_DEDUP_CAPACITY; i++)
	{
		if ((g_mesh.beacon_dedup[i].valid != 0U) &&
		    (APP_MeshTimeReached(now, g_mesh.beacon_dedup_expiry[i]) == 0U) &&
		    APP_MeshBeaconKeyEqual(&g_mesh.beacon_dedup[i].key, key))
		{
			if (g_mesh.beacon_dedup_count[i] != 0xFFU) g_mesh.beacon_dedup_count[i]++;
			if ((g_mesh.beacon_dedup_count[i] == 2U) ||
			    ((g_mesh.beacon_dedup_count[i] & 0x0FU) == 0U))
			{
				APP_MESH_TRACE("MESH", "DEDUP_HIT key=%04X:%u:%02X origin=%s count=%u action=NO_EXEC_NO_RELAY",
				               (unsigned int)key->network_id, (unsigned int)key->seq,
				               (unsigned int)key->cmd, origin,
				               (unsigned int)g_mesh.beacon_dedup_count[i]);
			}
			return 1U;
		}
		if ((g_mesh.beacon_dedup[i].valid == 0U) ||
		    APP_MeshTimeReached(now, g_mesh.beacon_dedup_expiry[i]) != 0U)
			slot = i;
	}
	if (slot >= APP_MESH_DEDUP_CAPACITY) slot = 0U;
	g_mesh.beacon_dedup[slot].valid = 1U;
	g_mesh.beacon_dedup[slot].key = *key;
	g_mesh.beacon_dedup_expiry[slot] = now + APP_MESH_MS_TO_TICKS(APP_MESH_DEDUP_TTL_MS);
	g_mesh.beacon_dedup_count[slot] = 1U;
	g_mesh.beacon_dedup_cursor = (uint8_t)((slot + 1U) % APP_MESH_DEDUP_CAPACITY);
	return 0U;
}

static uint8_t APP_MeshRemoteDedup(const app_mesh_remote_key_t *key, uint32_t now)
{
	uint8_t i;
	uint8_t slot = g_mesh.remote_dedup_cursor;

	for (i = 0U; i < APP_MESH_DEDUP_CAPACITY; i++)
	{
		if ((g_mesh.remote_dedup_valid[i] != 0U) &&
		    (APP_MeshTimeReached(now, g_mesh.remote_dedup_expiry[i]) == 0U) &&
		    APP_MeshLogicRemoteKeyEqual(&g_mesh.remote_dedup[i], key))
		{
			if (g_mesh.remote_dedup_count[i] != 0xFFU) g_mesh.remote_dedup_count[i]++;
			if ((g_mesh.remote_dedup_count[i] == 2U) ||
			    ((g_mesh.remote_dedup_count[i] & 0x0FU) == 0U))
			{
				APP_MESH_TRACE("MESH", "REMOTE_DEDUP_HIT key=%04X:%u:%02X:%02X:%02X count=%u action=NO_CONSUME_NO_RELAY",
				               (unsigned int)key->address, (unsigned int)key->count,
				               (unsigned int)key->cmd, (unsigned int)key->cmd_type,
				               (unsigned int)key->para,
				               (unsigned int)g_mesh.remote_dedup_count[i]);
			}
			return 1U;
		}
		if ((g_mesh.remote_dedup_valid[i] == 0U) ||
		    APP_MeshTimeReached(now, g_mesh.remote_dedup_expiry[i]) != 0U)
			slot = i;
	}
	if (slot >= APP_MESH_DEDUP_CAPACITY) slot = 0U;
	g_mesh.remote_dedup_valid[slot] = 1U;
	g_mesh.remote_dedup[slot] = *key;
	g_mesh.remote_dedup_expiry[slot] = now + APP_MESH_MS_TO_TICKS(APP_MESH_DEDUP_TTL_MS);
	g_mesh.remote_dedup_count[slot] = 1U;
	g_mesh.remote_dedup_cursor = (uint8_t)((slot + 1U) % APP_MESH_DEDUP_CAPACITY);
	return 0U;
}

static uint8_t APP_MeshBuildLeaderBeacon(uint8_t cmd, uint8_t flags,
	                                     uint8_t seq, uint8_t out[APP_BEACON_LENGTH])
{
	app_mesh_beacon_t beacon;

	memset(&beacon, 0, sizeof(beacon));
	beacon.network_id = APP_BLE_NETWORK_ID;
	beacon.seq = seq;
	beacon.cmd = cmd;
	beacon.flags = flags;
	memcpy(beacon.payload, g_mesh.election_id, 6U);
	if ((cmd == APP_BEACON_CMD_LEADER_ADV) || (cmd == APP_BEACON_CMD_SYNC_TICK))
		memcpy(&beacon.payload[6], &g_mesh.sync_payload[6], 13U);
	else
		beacon.payload[6] = 0U;
	APP_MeshLogicEncodeBeacon(&beacon, out);
	return 1U;
}

static uint8_t APP_MeshQueueLeaderFrame(mesh_tx_type_t type, uint8_t cmd,
	                                    uint8_t priority, uint32_t due_at,
	                                    uint32_t expire_at)
{
	uint8_t encoded[APP_BEACON_LENGTH];
	mesh_beacon_key_t key;

	(void)APP_MeshBuildLeaderBeacon(cmd, 0x01U, APP_MeshNextSeq(), encoded);
	key.network_id = APP_BLE_NETWORK_ID;
	key.seq = encoded[2];
	key.cmd = cmd;
	return APP_MeshQueueBeacon(type, priority, encoded, due_at, expire_at, &key,
	                           (const app_mesh_remote_key_t *)0);
}

static uint8_t APP_MeshCanSendMacQuery(uint32_t now)
{
	return APP_MeshElapsed(now, g_mesh.last_uart_tx,
	                       APP_MESH_MS_TO_TICKS(APP_MESH_UART_MIN_MS));
}

static void APP_MeshSendFrame(uint8_t command, const uint8_t *payload,
	                          uint16_t payload_len)
{
	uint16_t frame_len;

	frame_len = APP_MeshLogicEncodeUartFrame(APP_UART_HEAD_MCU_H,
	                                         APP_UART_HEAD_MCU_L,
	                                         command, payload, payload_len,
	                                         g_Usart1TxBuf, USART1_TXBUFF_SIZE);
	if (frame_len == 0U) return;
	APP_UsartTransmit(USART1, g_Usart1TxBuf, frame_len);
	g_mesh.last_uart_tx = wos;
}

static void APP_MeshSendMacQuery(uint32_t now)
{
	if (APP_MeshCanSendMacQuery(now) == 0U) return;
	APP_MeshSendFrame(APP_UART_CMD_MAC, (const uint8_t *)0, 0U);
	g_mesh.mac_query_pending = 1U;
	g_mesh.last_mac_query = wos;
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[BLE] TX MAC query 0x71\r\n");
#endif
}

static int8_t APP_MeshSelectTx(uint32_t now)
{
	int8_t selected = -1;
	uint8_t i;

	for (i = 0U; i < APP_MESH_TX_QUEUE_CAPACITY; i++)
	{
		mesh_tx_item_t *item = &g_mesh.tx_queue[i];
		if (item->active == 0U) continue;
		if (APP_MeshTimeReached(now, item->expire_at) != 0U)
		{
			APP_MeshQueueDrop(item, MESH_TX_EXPIRED, "SEND_DEADLINE");
			continue;
		}
		if (APP_MeshTimeReached(now, item->due_at) == 0U) continue;
		if ((selected < 0) ||
		    (item->priority < g_mesh.tx_queue[(uint8_t)selected].priority) ||
		    ((item->priority == g_mesh.tx_queue[(uint8_t)selected].priority) &&
		     (item->created_at < g_mesh.tx_queue[(uint8_t)selected].created_at)))
			selected = (int8_t)i;
	}
	return selected;
}

static void APP_MeshProcessTxQueue(uint32_t now)
{
	int8_t selected;
	mesh_tx_item_t *item;
	uint32_t actual;
	uint32_t queued_delay_ms;

	/* Always schedule against the current tick, not APP_MeshTick's old sample. */
	now = wos;

	if (g_mesh.module_ready == 0U) return;
	selected = APP_MeshSelectTx(now);
	if (selected < 0) return;
	item = &g_mesh.tx_queue[(uint8_t)selected];
	if (APP_MeshTimeReached(now, g_mesh.beacon_hold_until) == 0U)
	{
		if (APP_MeshElapsed(now, item->last_wait_log, APP_TICKS_500MS) != 0U)
		{
			APP_MESH_TRACE("TXQ", "WAIT type=%s reason=BLE_HOLD remaining_ms=%lu",
			               APP_MeshTxName(item->type),
			               (unsigned long)((uint64_t)(g_mesh.beacon_hold_until - now) * USER_WOS_TICK_US / 1000ULL));
			item->last_wait_log = now;
		}
		return;
	}
	if (APP_MeshElapsed(now, g_mesh.last_uart_tx,
	                   APP_MESH_MS_TO_TICKS(APP_MESH_UART_MIN_MS)) == 0U)
	{
		if (APP_MeshElapsed(now, item->last_wait_log, APP_TICKS_500MS) != 0U)
		{
			APP_MESH_TRACE("TXQ", "WAIT type=%s reason=UART_MIN remaining_ms=%lu",
			               APP_MeshTxName(item->type),
			               (unsigned long)((uint64_t)(g_mesh.last_uart_tx + APP_MESH_MS_TO_TICKS(APP_MESH_UART_MIN_MS) - now) * USER_WOS_TICK_US / 1000ULL));
			item->last_wait_log = now;
		}
		return;
	}
	if (item->type == MESH_TX_RELAY)
	{
		queued_delay_ms = APP_MeshNonNegativeDeltaMs(now, item->created_at);
		if (item->has_remote_key != 0U)
		{
			APP_MESH_TRACE("RELAY", "SEND kind=REMOTE_83 key=%04X:%u:%02X:%02X:%02X flags=0x%02X queued_delay_ms=%lu",
			               (unsigned int)item->remote_key.address,
			               (unsigned int)item->remote_key.count,
			               (unsigned int)item->remote_key.cmd,
			               (unsigned int)item->remote_key.cmd_type,
			               (unsigned int)item->remote_key.para,
			               (unsigned int)item->beacon[4],
			               (unsigned long)queued_delay_ms);
		}
		else
		{
			APP_MESH_TRACE("RELAY", "SEND kind=BEACON key=%04X:%u:%02X flags=0x%02X queued_delay_ms=%lu",
			               (unsigned int)item->beacon_key.network_id,
			               (unsigned int)item->beacon_key.seq,
			               (unsigned int)item->beacon_key.cmd,
			               (unsigned int)item->beacon[4],
			               (unsigned long)queued_delay_ms);
		}
	}
	APP_MeshSendFrame(APP_UART_CMD_BEACON_TX, item->beacon, APP_BEACON_LENGTH);
	actual = wos;
	g_mesh.beacon_hold_until = actual + APP_TICKS_400MS;
	item->state = MESH_TX_SENT;
	APP_MESH_TRACE("TXQ", "SEND type=%s seq=%u queued_ms=%lu actual_ms=%lu delay_ms=%lu result=OK",
	               APP_MeshTxName(item->type), (unsigned int)item->beacon[2],
	               (unsigned long)APP_MeshTicksToMs(item->created_at),
	               (unsigned long)APP_MeshTicksToMs(actual),
	               (unsigned long)APP_MeshNonNegativeDeltaMs(actual, item->created_at));
	item->active = 0U;
}

static uint8_t APP_MeshParseRemote(const uint8_t *payload,
	                               app_mesh_remote_key_t *remote)
{
	uint8_t xor_value = 0U;
	uint8_t i;

	if (memcmp(payload, g_remote_cid, 4U) != 0) return 0U;
	if ((payload[4] != 0x0BU) || (payload[5] != 0xFFU) || (payload[7] != 0x02U)) return 0U;
	if ((payload[6] != 0xCCU) && (payload[6] != 0xDDU) && (payload[6] != 0x99U)) return 0U;
	for (i = 0U; i < 15U; i++) xor_value ^= payload[i];
	if (xor_value != payload[15]) return 0U;
	remote->count = payload[8];
	remote->address = (uint16_t)(((uint16_t)payload[9] << 8) | payload[10]);
	remote->cmd = payload[11];
	remote->cmd_type = payload[12];
	remote->para = payload[13];
	return 1U;
}

static void APP_MeshConsumeRemote(const app_mesh_remote_key_t *remote,
                              uint8_t from_relay)
{
	(void)remote;
	(void)from_relay;
	APP_MESH_TRACE("REMOTE", "CONSUME addr=0x%04X count=0x%02X cmd=0x%02X type=%s para=0x%02X dedup=NEW source=%s",
	               (unsigned int)remote->address, (unsigned int)remote->count,
	               (unsigned int)remote->cmd, APP_MeshRemoteTypeName(remote->cmd_type),
	               (unsigned int)remote->para, (from_relay != 0U) ? "RELAY_83" : "RAW");
	if (g_mesh.role == MESH_ROLE_LEADER) APP_BleMeshNotifySyncStateChanged();
}

static void APP_MeshHandleRemoteFrame(const uint8_t *payload, uint16_t payload_len)
{
	app_mesh_remote_key_t remote;
	uint32_t relay_delay;

	if (payload_len != APP_REMOTE_LENGTH) return;
	if (APP_MeshParseRemote(payload, &remote) == 0U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[REMOTE] invalid frame CID/Len/SigType/Version/XOR\r\n");
#endif
		return;
	}
	if (remote.address != APP_BLE_NETWORK_ID)
	{
		APP_MESH_TRACE("REMOTE", "DROP addr=0x%04X reason=NETWORK_MISMATCH expected=0x%04X",
		               (unsigned int)remote.address, (unsigned int)APP_BLE_NETWORK_ID);
		return;
	}
	if (remote.cmd == 0xFFU)
	{
		APP_MESH_TRACE("REMOTE", "DROP addr=0x%04X count=0x%02X cmd=0xFF reason=CONFIG_IGNORED",
		               (unsigned int)remote.address, (unsigned int)remote.count);
		return;
	}
	if (APP_MeshRemoteDedup(&remote, wos) != 0U)
	{
		/* A duplicate raw controller frame is also evidence that a peer may
		 * already be carrying this command.  Do not leave our own 0x83 relay
		 * pending in that case. */
		APP_MeshCancelRelayRemote(&remote, "REMOTE_DUPLICATE");
		return;
	}
	APP_MESH_TRACE("REMOTE", "RX role=%s addr=0x%04X count=0x%02X cmd=0x%02X type=%s para=0x%02X dedup=NEW action=%s",
	               APP_MeshRoleName(g_mesh.role), (unsigned int)remote.address,
	               (unsigned int)remote.count, (unsigned int)remote.cmd,
	               APP_MeshRemoteTypeName(remote.cmd_type), (unsigned int)remote.para,
	               (g_mesh.role == MESH_ROLE_LEADER) ? "CONSUME" : "NO_LOCAL_EXEC");
	if (g_mesh.role == MESH_ROLE_LEADER)
	{
		APP_MeshConsumeRemote(&remote, 0U);
	}
	else if (g_mesh.role == MESH_ROLE_FOLLOWER)
	{
		if (APP_MeshScheduleRemoteRelay(&remote, &relay_delay) != 0U)
		{
			APP_MESH_TRACE("REMOTE", "ENQUEUE_83 addr=0x%04X count=0x%02X cmd=0x%02X flags=0x03 para_len=1 para=0x%02X delay_ms=%lu",
			               (unsigned int)remote.address, (unsigned int)remote.count,
			               (unsigned int)remote.cmd, (unsigned int)remote.para,
			               (unsigned long)((uint64_t)relay_delay * USER_WOS_TICK_US / 1000ULL));
		}
	}
}

static uint8_t APP_MeshValidateBeaconSemantics(const app_mesh_beacon_t *beacon)
{
	uint8_t i;

	if (beacon->network_id != APP_BLE_NETWORK_ID) return 0U;
	if ((beacon->cmd < APP_BEACON_CMD_LEADER_ADV) ||
	    (beacon->cmd > APP_BEACON_CMD_REMOTE_RELAY)) return 0U;
	if (((beacon->flags & 0x02U) != 0U) &&
	    ((beacon->flags & 0x01U) == 0U)) return 0U;
	if (beacon->cmd == APP_BEACON_CMD_REMOTE_RELAY)
	{
		if ((beacon->flags != 0x03U) || (beacon->payload[0] != beacon->seq) ||
		    (beacon->payload[3] != 1U) || (beacon->payload[1] == 0xFFU)) return 0U;
		for (i = 5U; i < APP_MESH_BEACON_PAYLOAD_LENGTH; i++)
		{
			if (beacon->payload[i] != 0U) return 0U;
		}
	}
	return 1U;
}

static void APP_MeshHandleLeaderBeacon(const app_mesh_beacon_t *beacon,
	                                   const uint8_t source_id[6], uint32_t now)
{
	if (g_mesh.role == MESH_ROLE_FOLLOWER)
	{
		g_mesh.heartbeat_led_start = now;
		g_mesh.heartbeat_led_active = 1U;
	}
	if (g_mesh.role == MESH_ROLE_LEADER)
	{
		if (APP_MeshCompareId(source_id, g_mesh.election_id) < 0)
		{
			memcpy(g_mesh.current_leader, source_id, 6U);
			g_mesh.has_current_leader = 1U;
			g_mesh.last_leader_seen = now;
			APP_MeshSetRole(MESH_ROLE_FOLLOWER, "SMALLER_LEADER_ID");
		}
		else
		{
			APP_MESH_TRACE("MESH", "MULTIPLE_LEADER_DETECTED remote=%02X%02X%02X%02X%02X%02X local=%02X%02X%02X%02X%02X%02X",
			               source_id[0], source_id[1], source_id[2], source_id[3], source_id[4], source_id[5],
			               g_mesh.election_id[0], g_mesh.election_id[1], g_mesh.election_id[2],
			               g_mesh.election_id[3], g_mesh.election_id[4], g_mesh.election_id[5]);
		}
		return;
	}
	memcpy(g_mesh.current_leader, source_id, 6U);
	g_mesh.has_current_leader = 1U;
	g_mesh.last_leader_seen = now;
	if (g_mesh.role == MESH_ROLE_CANDIDATE) APP_MeshSetRole(MESH_ROLE_FOLLOWER, "LEADER_BEACON_RECEIVED");
	(void)beacon;
}

static void APP_MeshHandleBeaconFrame(const uint8_t *payload, uint16_t payload_len)
{
	app_mesh_beacon_t beacon;
	mesh_beacon_key_t key;
	uint8_t source_id[6];
	uint8_t relay_allowed;
	uint8_t relayed;
	uint32_t now = wos;

	if (payload_len != APP_BEACON_LENGTH) return;
	if (APP_MeshLogicDecodeBeacon(payload, &beacon) == 0U)
	{
	#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] RX Beacon checksum error\r\n");
	#endif
		APP_MESH_TRACE("MESH", "RX checksum=BAD");
		return;
	}
	if (beacon.network_id != APP_BLE_NETWORK_ID)
	{
	#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] RX Beacon ignored: NetworkID=0x%04X expected=0x%04X\r\n",
		       (unsigned int)beacon.network_id, (unsigned int)APP_BLE_NETWORK_ID);
	#endif
		APP_MESH_TRACE("MESH", "RX network=0x%04X result=DROP reason=NETWORK_MISMATCH",
		               (unsigned int)beacon.network_id);
		return;
	}
	if (APP_MeshValidateBeaconSemantics(&beacon) == 0U)
	{
		APP_MESH_TRACE("MESH", "RX network=0x%04X cmd=0x%02X flags=0x%02X result=DROP reason=INVALID_SEMANTICS",
		               (unsigned int)beacon.network_id, (unsigned int)beacon.cmd,
		               (unsigned int)beacon.flags);
		return;
	}
	key.network_id = beacon.network_id;
	key.seq = beacon.seq;
	key.cmd = beacon.cmd;
	relay_allowed = (beacon.flags & 0x01U) ? 1U : 0U;
	relayed = (beacon.flags & 0x02U) ? 1U : 0U;

	if (beacon.cmd == APP_BEACON_CMD_REMOTE_RELAY)
	{
		app_mesh_remote_key_t remote;
		remote.address = beacon.network_id;
		remote.count = beacon.payload[0];
		remote.cmd = beacon.payload[1];
		remote.cmd_type = beacon.payload[2];
		remote.para = beacon.payload[4];
		if (APP_MeshBeaconDedup(&key, now, "RELAYED") != 0U)
		{
			APP_MeshCancelRelayRemote(&remote, "RELAYED_DUPLICATE");
			return;
		}
		APP_MeshCancelRelayRemote(&remote, "RELAYED_DUPLICATE");
		{
			uint8_t remote_duplicate = APP_MeshRemoteDedup(&remote, now);
		APP_MESH_TRACE("REMOTE", "RX_83 role=%s addr=0x%04X count=0x%02X cmd=0x%02X para_len=1 para=0x%02X flags=0x%02X",
		               APP_MeshRoleName(g_mesh.role), (unsigned int)remote.address,
		               (unsigned int)remote.count, (unsigned int)remote.cmd,
		               (unsigned int)remote.para, (unsigned int)beacon.flags);
		if ((g_mesh.role == MESH_ROLE_LEADER) && (remote_duplicate == 0U))
			APP_MeshConsumeRemote(&remote, 1U);
		}
		return;
	}

	memcpy(source_id, beacon.payload, 6U);
	if (APP_MeshIsZeroId(source_id) != 0U)
	{
		APP_MESH_TRACE("MESH", "RX cmd=%s result=DROP reason=INVALID_ELECTION_ID", APP_MeshCmdName(beacon.cmd));
		return;
	}
	if ((g_mesh.mac_ready != 0U) && (memcmp(source_id, g_mesh.election_id, 6U) == 0))
	{
		APP_MESH_TRACE("MESH", "RX cmd=%s result=DROP reason=SELF_LOOP", APP_MeshCmdName(beacon.cmd));
		return;
	}
	if (APP_MeshBeaconDedup(&key, now,
	                       (relayed != 0U) ? "RELAYED" : "RAW") != 0U)
	{
		if (relayed != 0U) APP_MeshCancelRelayBeacon(&key, "RELAYED_DUPLICATE");
		return;
	}
	APP_MESH_TRACE("MESH", "RX network=0x%04X cmd=%s seq=%u flags=0x%02X origin=%s source=%02X%02X%02X%02X%02X%02X key=%04X:%u:%02X checksum=OK heartbeat=%s sync=%s",
	               (unsigned int)beacon.network_id, APP_MeshCmdName(beacon.cmd),
	               (unsigned int)beacon.seq, (unsigned int)beacon.flags,
	               (relayed != 0U) ? "RELAYED" : "RAW",
	               source_id[0], source_id[1], source_id[2], source_id[3], source_id[4], source_id[5],
	               (unsigned int)key.network_id,
	               (unsigned int)key.seq, (unsigned int)key.cmd,
               (beacon.cmd == APP_BEACON_CMD_LEADER_RESIGN) ? "NO" : "REFRESH",
               (beacon.cmd == APP_BEACON_CMD_SYNC_TICK) ? "YES" : "NO");

	if (beacon.cmd == APP_BEACON_CMD_LEADER_RESIGN)
	{
		if ((g_mesh.has_current_leader != 0U) &&
		    (memcmp(beacon.payload, g_mesh.current_leader, 6U) == 0))
		{
			memset(g_mesh.current_leader, 0, sizeof(g_mesh.current_leader));
			g_mesh.has_current_leader = 0U;
			APP_MESH_TRACE("MESH", "RESIGN match=YES reason=0x%02X action=START_CANDIDATE",
			               (unsigned int)beacon.payload[6]);
			if (g_mesh.role == MESH_ROLE_FOLLOWER)
			{
				g_mesh.candidate_started = now;
				g_mesh.candidate_delay = APP_MeshCandidateDelayTicks();
				APP_MeshSetRole(MESH_ROLE_CANDIDATE, "LEADER_RESIGN");
			}
		}
		else
		{
			APP_MESH_TRACE("MESH", "RESIGN match=NO action=KEEP_STATE");
		}
	}
	else
	{
		APP_MeshHandleLeaderBeacon(&beacon, source_id, now);
	}
	if ((relay_allowed != 0U) && (relayed == 0U))
	{
		(void)APP_MeshScheduleBeaconRelay(payload, &key);
	}
	else if (relayed != 0U)
	{
		APP_MESH_TRACE("RELAY", "SUPPRESS key=%04X:%u:%02X reason=ALREADY_RELAYED",
		               (unsigned int)key.network_id, (unsigned int)key.seq,
		               (unsigned int)key.cmd);
	}
}

static void APP_MeshBecomeLeader(uint32_t now)
{
	memcpy(g_mesh.current_leader, g_mesh.election_id, 6U);
	g_mesh.has_current_leader = 1U;
	APP_MeshSetRole(MESH_ROLE_LEADER, "CANDIDATE_DELAY");
	/* The caller's leader branch queues the phase-zero ADV exactly once. */
	g_mesh.next_adv_due = now;
	g_mesh.next_sync_due = now + APP_MESH_MS_TO_TICKS(APP_MESH_SYNC_PHASE_MS);
}

static void APP_MeshCandidateStart(uint32_t now, const char *reason)
{
	g_mesh.candidate_started = now;
	g_mesh.candidate_delay = APP_MeshCandidateDelayTicks();
	APP_MeshSetRole(MESH_ROLE_CANDIDATE, reason);
	APP_MESH_TRACE("MESH", "CANDIDATE start_ms=%lu end_ms=%lu delay_ms=%lu formula=300+(id_last%%200)",
	               (unsigned long)((uint64_t)now * USER_WOS_TICK_US / 1000ULL),
	               (unsigned long)((uint64_t)(now + g_mesh.candidate_delay) * USER_WOS_TICK_US / 1000ULL),
	               (unsigned long)((uint64_t)g_mesh.candidate_delay * USER_WOS_TICK_US / 1000ULL));
}

static void APP_MeshTick(void)
{
	uint32_t now = wos;

	if (g_mesh.mac_ready == 0U)
	{
		if ((g_mesh.mac_query_pending == 0U) &&
		    ((g_mesh.module_ready != 0U) ||
		     APP_MeshElapsed(now, g_mesh.handshake_started, APP_TICKS_MODULE_QUERY_RETRY) != 0U))
		{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
			if (g_mesh.module_ready == 0U) printf("[BLE] startup MAC query fallback\r\n");
#endif
			APP_MeshSendMacQuery(now);
		}
		else if ((g_mesh.mac_query_pending != 0U) &&
		         APP_MeshElapsed(now, g_mesh.last_mac_query, APP_TICKS_MODULE_QUERY_RETRY) != 0U)
			APP_MeshSendMacQuery(now);
		APP_MeshLedUpdate(now);
		return;
	}

	if ((g_mesh.role == MESH_ROLE_FOLLOWER) &&
	    APP_MeshElapsed(now, g_mesh.last_leader_seen, APP_TICKS_3000MS) != 0U)
		APP_MeshCandidateStart(now, "LEADER_TIMEOUT");
	if ((g_mesh.role == MESH_ROLE_CANDIDATE) &&
	    APP_MeshElapsed(now, g_mesh.candidate_started, g_mesh.candidate_delay) != 0U)
		APP_MeshBecomeLeader(now);
	if (g_mesh.role == MESH_ROLE_LEADER)
	{
		if (APP_MeshTimeReached(now, g_mesh.next_adv_due) != 0U)
		{
			(void)APP_MeshQueueLeaderFrame(MESH_TX_ADV, APP_BEACON_CMD_LEADER_ADV, 2U,
			                               now, now + APP_TICKS_400MS + APP_MESH_MS_TO_TICKS(100UL));
			g_mesh.next_adv_due += APP_TICKS_ADV_PERIOD;
			if (APP_MeshTimeReached(now, g_mesh.next_adv_due) != 0U)
				g_mesh.next_adv_due = now + APP_TICKS_ADV_PERIOD;
		}
		if ((g_mesh.sync_dynamic != 0U) && APP_MeshTimeReached(now, g_mesh.next_sync_due) != 0U)
		{
			(void)APP_MeshQueueLeaderFrame(MESH_TX_SYNC, APP_BEACON_CMD_SYNC_TICK, 4U,
			                               now, now + APP_TICKS_400MS + APP_MESH_MS_TO_TICKS(100UL));
			g_mesh.next_sync_due += APP_TICKS_SYNC_PERIOD;
			if (APP_MeshTimeReached(now, g_mesh.next_sync_due) != 0U)
				g_mesh.next_sync_due = now + APP_TICKS_SYNC_PERIOD;
		}
	}
	APP_MeshProcessRelay(now);
	APP_MeshProcessTxQueue(now);
	APP_MeshLedUpdate(now);
}

static uint8_t APP_ParseUartFrame(uint8_t *buf, uint16_t *frame_len,
	                              uint8_t *command, uint8_t **payload,
	                              uint16_t *payload_len)
{
	uint16_t i;
	uint16_t start = 0U;
	uint16_t length;
	uint16_t total;
	uint8_t found = 0U;

	if (*frame_len < 7U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid frame: too short len=%u\r\n", (unsigned int)*frame_len);
#endif
		return 0U;
	}
	for (i = 0U; (i + 1U) < *frame_len; i++)
	{
		if (((buf[i] == APP_UART_HEAD_MODULE_H) && (buf[i + 1U] == APP_UART_HEAD_MODULE_L)) ||
		    ((buf[i] == APP_UART_HEAD_MCU_H) && (buf[i + 1U] == APP_UART_HEAD_MCU_L)))
		{
			start = i;
			found = 1U;
			break;
		}
	}
	if (found == 0U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid frame: no valid header\r\n");
#endif
		return 0U;
	}
	if (start > 0U)
	{
		memmove(buf, &buf[start], (size_t)(*frame_len - start));
		*frame_len = (uint16_t)(*frame_len - start);
	}
	length = (uint16_t)(((uint16_t)buf[3] << 8) | buf[4]);
	if (length > (uint16_t)(USART1_RXBUFF_SIZE - 7U))
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid payload length=%u max=%u\r\n",
		       (unsigned int)length, (unsigned int)(USART1_RXBUFF_SIZE - 7U));
#endif
		return 0U;
	}
	total = (uint16_t)(length + 7U);
	if ((total != *frame_len) || (buf[total - 1U] != APP_UART_TAIL))
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid length/tail actual=%u expected=%u\r\n",
		       (unsigned int)*frame_len, (unsigned int)total);
#endif
		return 0U;
	}
	if (APP_MeshLogicChecksum(buf, (uint16_t)(5U + length)) != buf[5U + length])
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] UART checksum error calc=0x%02X recv=0x%02X\r\n",
		       (unsigned int)APP_MeshLogicChecksum(buf, (uint16_t)(5U + length)),
		       (unsigned int)buf[5U + length]);
#endif
		return 0U;
	}
	*command = buf[2];
	*payload_len = length;
	*payload = &buf[5];
	return 1U;
}

void APP_BleMeshSetSyncMode(uint8_t dynamic)
{
	g_mesh.sync_dynamic = (dynamic != 0U) ? 1U : 0U;
	APP_MESH_TRACE("MESH", "SYNC_MODE dynamic=%u", (unsigned int)g_mesh.sync_dynamic);
}

void APP_BleMeshNotifySyncStateChanged(void)
{
	uint32_t now = wos;

	if (g_mesh.role == MESH_ROLE_LEADER)
	{
		(void)APP_MeshQueueLeaderFrame(MESH_TX_SYNC, APP_BEACON_CMD_SYNC_TICK, 4U,
		                               now, now + APP_TICKS_400MS + APP_MESH_MS_TO_TICKS(100UL));
		APP_MESH_TRACE("REMOTE", "SYNC_ENQUEUE reason=REMOTE_CONSUMED");
	}
}

void APP_BleMeshRequestResign(uint8_t reason)
{
	uint8_t encoded[APP_BEACON_LENGTH];
	app_mesh_beacon_t beacon;
	mesh_beacon_key_t key;
	uint32_t now = wos;

	if ((g_mesh.role != MESH_ROLE_LEADER) || (g_mesh.mac_ready == 0U)) return;
	memset(&beacon, 0, sizeof(beacon));
	beacon.network_id = APP_BLE_NETWORK_ID;
	beacon.seq = APP_MeshNextSeq();
	beacon.cmd = APP_BEACON_CMD_LEADER_RESIGN;
	beacon.flags = 0x01U;
	memcpy(beacon.payload, g_mesh.election_id, 6U);
	beacon.payload[6] = reason;
	APP_MeshLogicEncodeBeacon(&beacon, encoded);
	key.network_id = beacon.network_id;
	key.seq = beacon.seq;
	key.cmd = beacon.cmd;
	(void)APP_MeshQueueBeacon(MESH_TX_RESIGN, 1U, encoded, now,
	                          now + APP_TICKS_400MS + APP_MESH_MS_TO_TICKS(100UL), &key,
	                          (const app_mesh_remote_key_t *)0);
	APP_MeshCandidateStart(now, "LOCAL_RESIGN");
}

void APP_BleMeshInit(void)
{
	memset(&g_mesh, 0, sizeof(g_mesh));
	g_mesh.role = MESH_ROLE_INIT;
	g_mesh.sync_dynamic = 1U;
	g_mesh.sync_payload[6] = 1U;
	g_mesh.sync_payload[7] = 200U;
	g_mesh.led_start = wos;
	g_mesh.handshake_started = wos;
	g_mesh.last_mac_query = wos;
	g_mesh.last_leader_seen = wos;
	g_mesh.last_uart_tx = wos - APP_MESH_MS_TO_TICKS(APP_MESH_UART_MIN_MS);
	g_mesh.beacon_hold_until = wos;
	LEDB_H;
	APP_MESH_TRACE("MESH", "CONFIG network=0x1234 adv_ms=1000 sync_ms=1000 phase_ms=500 uart_min_ms=200 hold_ms=400 relay_ms=20..80 timeout_ms=3000 candidate_ms=300+(ElectionID[5]%%200) txq_capacity=8 relay_capacity=4 dedup_capacity=32 dedup_ttl_ms=1000");
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[BLE] Beacon Mesh init NetworkID=0x1234 waiting module ready\r\n");
#endif
}

void APP_HandleBleUartFrame(void)
{
	uint8_t frame[USART1_RXBUFF_SIZE];
	uint16_t frame_len;
	uint16_t copy_len;
	uint8_t command;
	uint8_t *payload;
	uint16_t payload_len;
	uint8_t mailbox_slot;
	uint8_t has_frame = 0U;

	if (g_Usart1FrameMailboxCount != 0U)
	{
		DISI();
		if (g_Usart1FrameMailboxCount != 0U)
		{
			mailbox_slot = g_Usart1FrameMailboxTail;
			copy_len = g_Usart1FrameMailboxLen[mailbox_slot];
			if (copy_len > USART1_RXBUFF_SIZE) copy_len = USART1_RXBUFF_SIZE;
			memcpy(frame, g_Usart1FrameMailbox[mailbox_slot], copy_len);
			g_Usart1FrameMailboxTail = (uint8_t)((mailbox_slot + 1U) %
			                                    APP_USART1_FRAME_MAILBOX_CAPACITY);
			g_Usart1FrameMailboxCount--;
			has_frame = 1U;
		}
		ENI();
	}
	if (has_frame != 0U)
	{
		frame_len = copy_len;
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG && APP_VERBOSE_UART1_RX
		printf("[UART1 RX] len=%u ", (unsigned int)frame_len);
		print_hex_array(frame, frame_len);
#endif
		if (APP_ParseUartFrame(frame, &frame_len, &command, &payload, &payload_len) != 0U)
		{
			if ((command == APP_UART_CMD_MODULE_READY) && (payload_len == 0U))
			{
				g_mesh.module_ready = 1U;
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
				printf("[BLE] module ready\r\n");
#endif
				if (g_mesh.mac_ready == 0U) APP_MeshSendMacQuery(wos);
			}
			else if ((command == APP_UART_CMD_MAC) && (payload_len == 6U))
			{
				if (APP_MeshIsZeroId(payload) == 0U)
				{
					uint8_t inferred_module_ready = (g_mesh.module_ready == 0U) ? 1U : 0U;
					memcpy(g_mesh.election_id, payload, 6U);
					memcpy(g_mesh.sync_payload, payload, 6U);
					g_mesh.mac_ready = 1U;
					g_mesh.module_ready = 1U;
					g_mesh.mac_query_pending = 0U;
					g_mesh.last_leader_seen = wos;
					#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
					printf("[BLE] MAC/ElectionID=%02X %02X %02X %02X %02X %02X\r\n",
					       payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
					if (inferred_module_ready != 0U)
						printf("[BLE] module ready inferred from MAC response\r\n");
					#endif
					APP_MeshSetRole(MESH_ROLE_FOLLOWER, "MAC_READY");
				}
				else
				{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
					printf("[BLE] invalid zero MAC; election blocked\r\n");
#endif
				}
			}
			else if ((command == APP_UART_CMD_BEACON_RX) && (payload_len == APP_REMOTE_LENGTH))
			{
				APP_MeshHandleRemoteFrame(payload, payload_len);
			}
				else if ((command == APP_UART_CMD_BEACON_RX) && (payload_len == APP_BEACON_LENGTH))
				{
					APP_MeshHandleBeaconFrame(payload, payload_len);
				}
				else
				{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
					printf("[UART1 RX] unsupported command=0x%02X payload_len=%u\r\n",
					       (unsigned int)command, (unsigned int)payload_len);
#endif
				}
			}
	}
	APP_MeshTick();
}
