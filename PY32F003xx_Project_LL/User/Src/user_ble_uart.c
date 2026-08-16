/**
  ******************************************************************************
  * @file    user_ble_uart.c
  * @brief   UART1 Beacon Mesh protocol, remote parser and leader state machine
  ******************************************************************************
  */

#include "user_ble_uart.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "user_bsp_uart1.h"
#include "user_bsp_uart2.h"
#include "user_bsp_gpio.h"
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
#define APP_UART_CMD_BEACON_RX      0x91U
#define APP_UART_CMD_BEACON_TX      0x92U
#define APP_BEACON_CMD_LEADER_ADV   0x80U
#define APP_BEACON_CMD_SYNC_TICK    0x81U
#define APP_BEACON_LENGTH            25U
#define APP_REMOTE_LENGTH            16U
#define APP_UART_HEAD_MODULE_H       0xAAU
#define APP_UART_HEAD_MODULE_L       0x55U
#define APP_UART_HEAD_MCU_H          0x55U
#define APP_UART_HEAD_MCU_L          0xAAU
#define APP_UART_TAIL                0xFEU

/* wos advances every 400 us. */
#define APP_TICKS_100MS              250UL
#define APP_TICKS_500MS              1250UL
#define APP_TICKS_200MS              500UL
#define APP_TICKS_1000MS             2500UL
#define APP_TICKS_2000MS             5000UL
#define APP_TICKS_POWER_ON_LED       7500UL
#define APP_TICKS_QUERY_RETRY        12500UL

static const uint8_t g_remote_cid[4] = {0x03U, 0x09U, 0x4CU, 0x5AU};

typedef enum
{
	MESH_ROLE_INIT = 0U,
	MESH_ROLE_FOLLOWER,
	MESH_ROLE_CANDIDATE,
	MESH_ROLE_LEADER
} mesh_role_t;

typedef struct
{
	uint16_t network_id;
	uint8_t seq;
	uint8_t cmd;
	uint8_t flags;
	uint8_t payload[19];
	uint8_t beacon_checksum;
} beacon_pkt_t;

typedef struct
{
	uint8_t sig_source;
	uint8_t version;
	uint8_t count;
	uint16_t address;
	uint8_t cmd;
	uint8_t cmd_type;
	uint8_t para;
	uint8_t rand;
	uint8_t check_valid;
} remote_pkt_t;

typedef struct
{
	mesh_role_t role;
	uint8_t module_ready;
	uint8_t mac_ready;
	uint8_t mac_query_sent;
	uint8_t election_id[6];
	uint8_t current_leader[6];
	uint8_t has_current_leader;
	uint8_t seq;
	uint32_t last_leader_seen;
	uint32_t candidate_started;
	uint32_t candidate_delay;
	uint32_t last_adv;
	uint32_t last_sync;
	uint32_t last_uart_tx;
	uint32_t last_mac_query;
	uint32_t led_start;
	uint32_t heartbeat_led_start;
	uint8_t heartbeat_led_active;
	uint8_t led_output;
} mesh_runtime_t;

static mesh_runtime_t g_mesh;

static uint8_t APP_BleFrameChecksum(const uint8_t *p, uint16_t byte_count)
{
	uint32_t sum = 0U;
	uint16_t i;

	for (i = 0U; i < byte_count; i++)
	{
		sum += (uint32_t)p[i];
	}
	return (uint8_t)(sum & 0xFFU);
}

static uint8_t APP_BleElapsed(uint32_t now, uint32_t since, uint32_t period)
{
	return ((uint32_t)(now - since) >= period) ? 1U : 0U;
}

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

static const char *APP_RemoteSourceName(uint8_t source)
{
	switch (source)
	{
	case 0xCCU: return "遥控器";
	case 0xDDU: return "APP";
	case 0x99U: return "测试遥控器";
	default:    return "未知来源";
	}
}

static const char *APP_RemoteCmdTypeName(uint8_t cmd_type)
{
	switch (cmd_type)
	{
	case 0x00U: return "短按";
	case 0x01U: return "长按";
	case 0x02U: return "抬起";
	case 0xAAU: return "测试";
	default:    return "未知类型";
	}
}

static void APP_LogId(const char *prefix, const uint8_t id[6])
{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("%s%02X %02X %02X %02X %02X %02X\r\n",
	       prefix, id[0], id[1], id[2], id[3], id[4], id[5]);
#else
	(void)prefix;
	(void)id;
#endif
}

static void APP_LogRoleTransition(mesh_role_t from, mesh_role_t to, const char *reason)
{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	if (from != to)
	{
		printf("[MESH] %s -> %s reason=%s\r\n",
		       APP_MeshRoleName(from), APP_MeshRoleName(to), reason);
	}
#else
	(void)from;
	(void)to;
	(void)reason;
#endif
}

static void APP_MeshLedSet(uint8_t on)
{
	on = (on != 0U) ? 1U : 0U;
	if (g_mesh.led_output == on)
	{
		return;
	}

	g_mesh.led_output = on;
	if (on != 0U)
	{
		LEDB_L;
	}
	else
	{
		LEDB_H;
	}
}

static void APP_MeshLedUpdate(uint32_t now)
{
	uint8_t led_on = 0U;
	uint32_t elapsed = (uint32_t)(now - g_mesh.led_start);

	if (elapsed < APP_TICKS_POWER_ON_LED)
	{
		/* Startup display has priority and starts with the LED off. */
		led_on = (uint8_t)((elapsed / APP_TICKS_500MS) & 1UL);
	}
	else if (g_mesh.role == MESH_ROLE_LEADER)
	{
		led_on = 1U;
	}
	else if ((g_mesh.role == MESH_ROLE_FOLLOWER) &&
	         (g_mesh.heartbeat_led_active != 0U))
	{
		if (APP_BleElapsed(now, g_mesh.heartbeat_led_start,
	                      APP_TICKS_100MS) != 0U)
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
	g_mesh.role = role;
	if (role != MESH_ROLE_FOLLOWER)
	{
		g_mesh.heartbeat_led_active = 0U;
	}
	APP_LogRoleTransition(old_role, role, reason);
}

static uint8_t APP_IsZeroId(const uint8_t id[6])
{
	uint8_t i;

	for (i = 0U; i < 6U; i++)
	{
		if (id[i] != 0U)
		{
			return 0U;
		}
	}
	return 1U;
}

static int8_t APP_CompareId(const uint8_t left[6], const uint8_t right[6])
{
	uint8_t i;

	for (i = 0U; i < 6U; i++)
	{
		if (left[i] < right[i]) return -1;
		if (left[i] > right[i]) return 1;
	}
	return 0;
}

static void APP_BleSendFrame(uint8_t command, const uint8_t *payload, uint16_t payload_len)
{
	uint16_t frame_len;

	if ((payload_len + 7U) > USART1_TXBUFF_SIZE)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[BLE] TX rejected: payload too long len=%u\r\n", (unsigned int)payload_len);
#endif
		return;
	}

	g_Usart1TxBuf[0] = APP_UART_HEAD_MCU_H;
	g_Usart1TxBuf[1] = APP_UART_HEAD_MCU_L;
	g_Usart1TxBuf[2] = command;
	g_Usart1TxBuf[3] = (uint8_t)(payload_len >> 8);
	g_Usart1TxBuf[4] = (uint8_t)(payload_len & 0xFFU);
	if ((payload != (const uint8_t *)0) && (payload_len > 0U))
	{
		memcpy(&g_Usart1TxBuf[5], payload, payload_len);
	}
	frame_len = (uint16_t)(payload_len + 5U);
	g_Usart1TxBuf[frame_len] = APP_BleFrameChecksum(g_Usart1TxBuf, frame_len);
	g_Usart1TxBuf[frame_len + 1U] = APP_UART_TAIL;
	APP_UsartTransmit(USART1, g_Usart1TxBuf, (uint16_t)(frame_len + 2U));
	g_mesh.last_uart_tx = wos;
}

static void APP_BleSendMacQuery(void)
{
	APP_BleSendFrame(APP_UART_CMD_MAC, (const uint8_t *)0, 0U);
	g_mesh.mac_query_sent = 1U;
	g_mesh.last_mac_query = wos;
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[BLE] TX MAC query 0x71\r\n");
#endif
}

static uint8_t APP_BleCanSendBeacon(uint32_t now)
{
	return (g_mesh.module_ready != 0U) &&
	       APP_BleElapsed(now, g_mesh.last_uart_tx, APP_TICKS_200MS);
}

static void APP_BleBuildAndSendBeacon(uint8_t command, uint32_t now)
{
	beacon_pkt_t beacon;
	uint8_t i;

	if (APP_BleCanSendBeacon(now) == 0U)
	{
		return;
	}

	beacon.network_id = APP_BLE_NETWORK_ID;
	g_mesh.seq++;
	if (g_mesh.seq == 0U)
	{
		g_mesh.seq = 1U;
	}
	beacon.seq = g_mesh.seq;
	beacon.cmd = command;
	beacon.flags = 0U;
	memset(beacon.payload, 0, sizeof(beacon.payload));
	memcpy(beacon.payload, g_mesh.election_id, 6U);
	beacon.payload[6] = 1U;
	beacon.payload[7] = 200U;

	g_Usart1TxBuf[0] = APP_UART_HEAD_MCU_H;
	g_Usart1TxBuf[1] = APP_UART_HEAD_MCU_L;
	g_Usart1TxBuf[2] = APP_UART_CMD_BEACON_TX;
	g_Usart1TxBuf[3] = 0U;
	g_Usart1TxBuf[4] = APP_BEACON_LENGTH;
	g_Usart1TxBuf[5] = (uint8_t)(beacon.network_id >> 8);
	g_Usart1TxBuf[6] = (uint8_t)(beacon.network_id & 0xFFU);
	g_Usart1TxBuf[7] = beacon.seq;
	g_Usart1TxBuf[8] = beacon.cmd;
	g_Usart1TxBuf[9] = beacon.flags;
	for (i = 0U; i < 19U; i++)
	{
		g_Usart1TxBuf[10U + i] = beacon.payload[i];
	}
	beacon.beacon_checksum = APP_BleFrameChecksum(&g_Usart1TxBuf[5], 24U);
	g_Usart1TxBuf[29] = beacon.beacon_checksum;
	g_Usart1TxBuf[30] = APP_BleFrameChecksum(g_Usart1TxBuf, 30U);
	g_Usart1TxBuf[31] = APP_UART_TAIL;
	APP_UsartTransmit(USART1, g_Usart1TxBuf, 32U);
	g_mesh.last_uart_tx = now;

#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[MESH] TX %s seq=%u network=0x%04X flags=0x%02X\r\n",
	       (command == APP_BEACON_CMD_LEADER_ADV) ? "LEADER_ADV" : "SYNC_TICK",
	       (unsigned int)beacon.seq, (unsigned int)beacon.network_id,
	       (unsigned int)beacon.flags);
#endif
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
	uint8_t checksum;

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
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] realign header offset=%u\r\n", (unsigned int)start);
#endif
	}

	length = (uint16_t)(((uint16_t)buf[3] << 8) | buf[4]);
	total = (uint16_t)(length + 7U);
	if ((total != *frame_len) || (buf[total - 1U] != APP_UART_TAIL))
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid length/tail actual=%u expected=%u\r\n",
		       (unsigned int)*frame_len, (unsigned int)total);
#endif
		return 0U;
	}

	checksum = APP_BleFrameChecksum(buf, (uint16_t)(5U + length));
	if (checksum != buf[5U + length])
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] UART checksum error calc=0x%02X recv=0x%02X\r\n",
		       (unsigned int)checksum, (unsigned int)buf[5U + length]);
#endif
		return 0U;
	}

	*command = buf[2];
	*payload_len = length;
	*payload = &buf[5];
	return 1U;
}

static uint8_t APP_ParseRemote(const uint8_t *payload, remote_pkt_t *remote)
{
	uint8_t xor_value = 0U;
	uint8_t i;

	if (memcmp(payload, g_remote_cid, 4U) != 0)
	{
		return 0U;
	}
	if ((payload[4] != 0x0BU) || (payload[5] != 0xFFU) || (payload[7] != 0x02U))
	{
		return 0U;
	}
	if ((payload[6] != 0xCCU) && (payload[6] != 0xDDU) && (payload[6] != 0x99U))
	{
		return 0U;
	}
	for (i = 0U; i < 15U; i++)
	{
		xor_value ^= payload[i];
	}
	if (xor_value != payload[15])
	{
		return 0U;
	}

	remote->sig_source = payload[6];
	remote->version = payload[7];
	remote->count = payload[8];
	remote->address = (uint16_t)(((uint16_t)payload[9] << 8) | payload[10]);
	remote->cmd = payload[11];
	remote->cmd_type = payload[12];
	remote->para = payload[13];
	remote->rand = payload[14];
	remote->check_valid = 1U;
	return 1U;
}

static void APP_HandleRemoteFrame(const uint8_t *payload, uint16_t payload_len)
{
	remote_pkt_t remote;

	if (payload_len != APP_REMOTE_LENGTH)
	{
		return;
	}
	if (APP_ParseRemote(payload, &remote) == 0U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[REMOTE] invalid frame CID/Len/SigType/Version/XOR\r\n");
#endif
		return;
	}

#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[REMOTE] source=%s addr=0x%04X cmd=0x%02X type=%s para=0x%02X rand=0x%02X check=OK\r\n",
	       APP_RemoteSourceName(remote.sig_source), (unsigned int)remote.address,
	       (unsigned int)remote.cmd, APP_RemoteCmdTypeName(remote.cmd_type),
	       (unsigned int)remote.para, (unsigned int)remote.rand);
	if (remote.cmd == 0xFFU)
	{
		printf("[REMOTE] Cmd=0xFF configuration ignored: fixed NetworkID=0x1234\r\n");
	}
#endif
}

static void APP_HandleBeaconFrame(const uint8_t *payload, uint16_t payload_len)
{
	uint8_t source_id[6];
	uint8_t i;
	uint8_t checksum;
	uint8_t command;
	uint16_t network_id;
	uint8_t flags;
	uint8_t seq;
	uint8_t was_follower;
	uint32_t now = wos;

	if (payload_len != APP_BEACON_LENGTH)
	{
		return;
	}
	checksum = APP_BleFrameChecksum(payload, 24U);
	if (checksum != payload[24])
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] RX Beacon checksum error calc=0x%02X recv=0x%02X\r\n",
		       (unsigned int)checksum, (unsigned int)payload[24]);
#endif
		return;
	}

	network_id = (uint16_t)(((uint16_t)payload[0] << 8) | payload[1]);
	if (network_id != APP_BLE_NETWORK_ID)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] RX Beacon ignored: NetworkID=0x%04X expected=0x%04X\r\n",
		       (unsigned int)network_id, (unsigned int)APP_BLE_NETWORK_ID);
#endif
		return;
	}

	command = payload[3];
	if ((command != APP_BEACON_CMD_LEADER_ADV) && (command != APP_BEACON_CMD_SYNC_TICK))
	{
		return;
	}
	for (i = 0U; i < 6U; i++)
	{
		source_id[i] = payload[5U + i];
	}
	if (APP_IsZeroId(source_id) != 0U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] RX Beacon ignored: invalid ElectionID\r\n");
#endif
		return;
	}
	if ((g_mesh.mac_ready != 0U) && (memcmp(source_id, g_mesh.election_id, 6U) == 0))
	{
		return;
	}

	seq = payload[2];
	flags = payload[4];
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[MESH] RX %s seq=%u source=%02X %02X %02X %02X %02X %02X flags=0x%02X\r\n",
	       (command == APP_BEACON_CMD_LEADER_ADV) ? "LEADER_ADV" : "SYNC_TICK",
	       (unsigned int)seq, source_id[0], source_id[1], source_id[2], source_id[3],
	       source_id[4], source_id[5], (unsigned int)flags);
#endif

	/* Only a valid heartbeat received while already following may flash the LED. */
	was_follower = (g_mesh.role == MESH_ROLE_FOLLOWER) ? 1U : 0U;
	if (was_follower != 0U)
	{
		g_mesh.heartbeat_led_start = now;
		g_mesh.heartbeat_led_active = 1U;
	}

	if (g_mesh.role == MESH_ROLE_LEADER)
	{
		if (APP_CompareId(source_id, g_mesh.election_id) < 0)
		{
			memcpy(g_mesh.current_leader, source_id, 6U);
			g_mesh.has_current_leader = 1U;
			g_mesh.last_leader_seen = now;
			APP_MeshSetRole(MESH_ROLE_FOLLOWER, "SMALLER_LEADER_ID");
			APP_LogId("[MESH] current Leader=", source_id);
		}
		else
		{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
			printf("[MESH] MULTIPLE_LEADER_DETECTED: remote ID is not smaller\r\n");
#endif
		}
		return;
	}

	memcpy(g_mesh.current_leader, source_id, 6U);
	g_mesh.has_current_leader = 1U;
	g_mesh.last_leader_seen = now;
	if (g_mesh.role == MESH_ROLE_CANDIDATE)
	{
		APP_MeshSetRole(MESH_ROLE_FOLLOWER, "LEADER_BEACON_RECEIVED");
	}
}

static void APP_MeshBecomeLeader(uint32_t now)
{
	memcpy(g_mesh.current_leader, g_mesh.election_id, 6U);
	g_mesh.has_current_leader = 1U;
	APP_MeshSetRole(MESH_ROLE_LEADER, "CANDIDATE_DELAY");
	g_mesh.last_adv = now;
	g_mesh.last_sync = now;
	if (APP_BleCanSendBeacon(now) != 0U)
	{
		APP_BleBuildAndSendBeacon(APP_BEACON_CMD_LEADER_ADV, now);
	}
}

static void APP_BleMeshTick(void)
{
	uint32_t now = wos;

	if (g_mesh.mac_ready == 0U)
	{
		if ((g_mesh.module_ready != 0U) && (g_mesh.mac_query_sent != 0U) &&
		    (APP_BleElapsed(now, g_mesh.last_mac_query, APP_TICKS_QUERY_RETRY) != 0U))
		{
			APP_BleSendMacQuery();
		}
		APP_MeshLedUpdate(now);
		return;
	}

	if ((g_mesh.role == MESH_ROLE_FOLLOWER) &&
	    (APP_BleElapsed(now, g_mesh.last_leader_seen, APP_TICKS_2000MS) != 0U))
	{
		g_mesh.candidate_started = now;
		g_mesh.candidate_delay = 250UL +
			(((uint32_t)g_mesh.election_id[5] % 100UL) * 250UL) / 100UL;
		APP_MeshSetRole(MESH_ROLE_CANDIDATE, "LEADER_TIMEOUT");
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[MESH] Candidate Delay=%lu ticks\r\n",
		       (unsigned long)g_mesh.candidate_delay);
#endif
	}

	if ((g_mesh.role == MESH_ROLE_CANDIDATE) &&
	    (APP_BleElapsed(now, g_mesh.candidate_started, g_mesh.candidate_delay) != 0U))
	{
		APP_MeshBecomeLeader(now);
	}

	if (g_mesh.role != MESH_ROLE_LEADER)
	{
		APP_MeshLedUpdate(now);
		return;
	}

	if ((APP_BleElapsed(now, g_mesh.last_adv, APP_TICKS_1000MS) != 0U) &&
	    (APP_BleCanSendBeacon(now) != 0U))
	{
		APP_BleBuildAndSendBeacon(APP_BEACON_CMD_LEADER_ADV, now);
		g_mesh.last_adv = now;
	}
	else if ((APP_BleElapsed(now, g_mesh.last_sync, APP_TICKS_200MS) != 0U) &&
	         (APP_BleCanSendBeacon(now) != 0U))
	{
		APP_BleBuildAndSendBeacon(APP_BEACON_CMD_SYNC_TICK, now);
		g_mesh.last_sync = now;
	}

	APP_MeshLedUpdate(now);
}

void APP_BleMeshInit(void)
{
	memset(&g_mesh, 0, sizeof(g_mesh));
	g_mesh.role = MESH_ROLE_INIT;
	g_mesh.led_start = wos;
	LEDB_H;
	g_mesh.last_leader_seen = wos;
	g_mesh.last_uart_tx = wos - APP_TICKS_200MS;
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

	if (g_Usart1FrameReady != 0U)
	{
		DISI();
		copy_len = g_Usart1FrameLen;
		if (copy_len > USART1_RXBUFF_SIZE)
		{
			copy_len = USART1_RXBUFF_SIZE;
		}
		memcpy(frame, g_Usart1FrameBuf, copy_len);
		g_Usart1FrameReady = 0U;
		ENI();

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
				if ((g_mesh.mac_query_sent == 0U) ||
				    (APP_BleElapsed(wos, g_mesh.last_mac_query, APP_TICKS_QUERY_RETRY) != 0U))
				{
					APP_BleSendMacQuery();
				}
			}
			else if ((command == APP_UART_CMD_MAC) && (payload_len == 6U))
			{
				if (APP_IsZeroId(payload) == 0U)
				{
					memcpy(g_mesh.election_id, payload, 6U);
					g_mesh.mac_ready = 1U;
					g_mesh.mac_query_sent = 0U;
					g_mesh.last_leader_seen = wos;
					APP_LogId("[BLE] MAC/ElectionID=", g_mesh.election_id);
					APP_MeshSetRole(MESH_ROLE_FOLLOWER, "MAC_READY");
				}
				else
				{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
					printf("[BLE] invalid zero MAC; election blocked\r\n");
#endif
				}
			}
			else if ((command == APP_UART_CMD_BEACON_RX) &&
			         (payload_len == APP_REMOTE_LENGTH))
			{
				APP_HandleRemoteFrame(payload, payload_len);
			}
			else if ((command == APP_UART_CMD_BEACON_RX) &&
			         (payload_len == APP_BEACON_LENGTH))
			{
				APP_HandleBeaconFrame(payload, payload_len);
			}
			else
			{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
				printf("[BLE] unsupported command=0x%02X payload_len=%u\r\n",
				       (unsigned int)command, (unsigned int)payload_len);
#endif
			}
		}
	}

	APP_BleMeshTick();
}
