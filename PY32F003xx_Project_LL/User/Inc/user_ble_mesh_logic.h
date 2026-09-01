#ifndef USER_BLE_MESH_LOGIC_H
#define USER_BLE_MESH_LOGIC_H

#include <stdint.h>
#include <string.h>

#define APP_MESH_BEACON_LENGTH 25U
#define APP_MESH_BEACON_PAYLOAD_LENGTH 19U
#define APP_MESH_REMOTE_RELAY_CMD 0x83U
#define APP_MESH_UART_TAIL 0xFEU
#define APP_MESH_FLAGS_TERMINAL 0x00U
#define APP_MESH_FLAGS_RELAYABLE 0x01U

typedef struct
{
	uint16_t network_id;
	uint8_t seq;
	uint8_t cmd;
	uint8_t flags;
	uint8_t payload[APP_MESH_BEACON_PAYLOAD_LENGTH];
	uint8_t beacon_checksum;
} app_mesh_beacon_t;

typedef struct
{
	uint16_t address;
	uint8_t count;
	uint8_t cmd;
	uint8_t cmd_type;
	uint8_t para;
} app_mesh_remote_key_t;

typedef struct
{
	uint16_t network_id;
	uint8_t seq;
	uint8_t cmd;
} app_mesh_packet_key_t;

static uint8_t APP_MeshLogicChecksum(const uint8_t *data, uint16_t length)
{
	uint32_t sum = 0U;
	uint16_t i;

	for (i = 0U; i < length; i++)
	{
		sum += data[i];
	}
	return (uint8_t)(sum & 0xFFU);
}

static void APP_MeshLogicEncodeBeacon(const app_mesh_beacon_t *beacon,
	                                  uint8_t out[APP_MESH_BEACON_LENGTH])
{
	uint8_t i;

	out[0] = (uint8_t)(beacon->network_id >> 8);
	out[1] = (uint8_t)(beacon->network_id & 0xFFU);
	out[2] = beacon->seq;
	out[3] = beacon->cmd;
	out[4] = beacon->flags;
	for (i = 0U; i < APP_MESH_BEACON_PAYLOAD_LENGTH; i++)
	{
		out[5U + i] = beacon->payload[i];
	}
	out[24] = APP_MeshLogicChecksum(out, 24U);
}

static uint16_t APP_MeshLogicEncodeUartFrame(uint8_t header_high,
	                                         uint8_t header_low,
	                                         uint8_t command,
	                                         const uint8_t *payload,
	                                         uint16_t payload_length,
	                                         uint8_t *out,
	                                         uint16_t out_capacity)
{
	uint16_t i;
	uint16_t checksum_index;

	if ((out == (uint8_t *)0) || (out_capacity < 7U) ||
	    (payload_length > (uint16_t)(out_capacity - 7U)))
	{
		return 0U;
	}
	out[0] = header_high;
	out[1] = header_low;
	out[2] = command;
	out[3] = (uint8_t)(payload_length >> 8);
	out[4] = (uint8_t)(payload_length & 0xFFU);
	for (i = 0U; i < payload_length; i++)
	{
		out[5U + i] = (payload != (const uint8_t *)0) ? payload[i] : 0U;
	}
	checksum_index = (uint16_t)(5U + payload_length);
	out[checksum_index] = APP_MeshLogicChecksum(out, checksum_index);
	out[checksum_index + 1U] = APP_MESH_UART_TAIL;
	return (uint16_t)(payload_length + 7U);
}

static uint8_t APP_MeshLogicDecodeBeacon(const uint8_t in[APP_MESH_BEACON_LENGTH],
	                                    app_mesh_beacon_t *beacon)
{
	if (APP_MeshLogicChecksum(in, 24U) != in[24])
	{
		return 0U;
	}

	beacon->network_id = (uint16_t)(((uint16_t)in[0] << 8) | in[1]);
	beacon->seq = in[2];
	beacon->cmd = in[3];
	beacon->flags = in[4];
	memcpy(beacon->payload, &in[5], APP_MESH_BEACON_PAYLOAD_LENGTH);
	beacon->beacon_checksum = in[24];
	return 1U;
}

static uint8_t APP_MeshLogicIsValidFlags(uint8_t flags)
{
	return (flags == APP_MESH_FLAGS_TERMINAL) ||
	       (flags == APP_MESH_FLAGS_RELAYABLE);
}

static uint8_t APP_MeshLogicBuildBeaconRelay(
	const uint8_t in[APP_MESH_BEACON_LENGTH],
	uint8_t out[APP_MESH_BEACON_LENGTH])
{
	app_mesh_beacon_t beacon;

	if ((APP_MeshLogicDecodeBeacon(in, &beacon) == 0U) ||
	    (beacon.flags != APP_MESH_FLAGS_RELAYABLE))
	{
		return 0U;
	}
	beacon.flags = APP_MESH_FLAGS_TERMINAL;
	APP_MeshLogicEncodeBeacon(&beacon, out);
	return 1U;
}

static uint8_t APP_MeshLogicPacketKeyEqual(const app_mesh_packet_key_t *left,
	                                       const app_mesh_packet_key_t *right)
{
	return (left->network_id == right->network_id) &&
	       (left->seq == right->seq) && (left->cmd == right->cmd);
}

static uint8_t APP_MeshLogicBuildRemoteRelay(const app_mesh_remote_key_t *remote,
	                                         uint8_t flags,
	                                         uint8_t out[APP_MESH_BEACON_LENGTH])
{
	app_mesh_beacon_t beacon;

	if ((remote == (const app_mesh_remote_key_t *)0) ||
	    (out == (uint8_t *)0) ||
	    (APP_MeshLogicIsValidFlags(flags) == 0U))
	{
		return 0U;
	}

	memset(&beacon, 0, sizeof(beacon));
	beacon.network_id = remote->address;
	beacon.seq = remote->count;
	beacon.cmd = APP_MESH_REMOTE_RELAY_CMD;
	beacon.flags = flags;
	beacon.payload[0] = remote->count;
	beacon.payload[1] = remote->cmd;
	beacon.payload[2] = remote->cmd_type;
	beacon.payload[3] = 1U;
	beacon.payload[4] = remote->para;
	APP_MeshLogicEncodeBeacon(&beacon, out);
	return 1U;
}

static uint8_t APP_MeshLogicRemoteKeyEqual(const app_mesh_remote_key_t *left,
	                                       const app_mesh_remote_key_t *right)
{
	return (left->address == right->address) &&
	       (left->count == right->count) &&
	       (left->cmd == right->cmd) &&
	       (left->cmd_type == right->cmd_type) &&
	       (left->para == right->para);
}

#endif /* USER_BLE_MESH_LOGIC_H */
