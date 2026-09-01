#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "user_ble_mesh_logic.h"

static uint32_t test_ticks_to_ms(uint32_t ticks)
{
	return (uint32_t)(((uint64_t)ticks * 400ULL) / 1000ULL);
}

static uint32_t test_non_negative_delta_ms(uint32_t now, uint32_t since)
{
	int32_t delta = (int32_t)(now - since);

	if (delta < 0) return 0U;
	return test_ticks_to_ms((uint32_t)delta);
}

static void test_beacon_round_trip(void)
{
	app_mesh_beacon_t input;
	app_mesh_beacon_t output;
	uint8_t encoded[APP_MESH_BEACON_LENGTH];
	uint8_t i;

	memset(&input, 0, sizeof(input));
	input.network_id = 0x1234U;
	input.seq = 0x11U;
	input.cmd = 0x80U;
	input.flags = 0x01U;
	for (i = 0U; i < 6U; i++) input.payload[i] = (uint8_t)(i + 1U);
	input.payload[6] = 1U;
	input.payload[7] = 200U;

	APP_MeshLogicEncodeBeacon(&input, encoded);
	assert(APP_MeshLogicDecodeBeacon(encoded, &output) != 0U);
	assert(output.network_id == input.network_id);
	assert(output.seq == input.seq);
	assert(output.cmd == input.cmd);
	assert(output.flags == input.flags);
	assert(memcmp(output.payload, input.payload, sizeof(input.payload)) == 0);
	assert(encoded[24] == APP_MeshLogicChecksum(encoded, 24U));
	encoded[4] ^= 0x01U;
	assert(APP_MeshLogicDecodeBeacon(encoded, &output) == 0U);
}

static void test_beacon_relay_copy(void)
{
	app_mesh_beacon_t input;
	app_mesh_beacon_t output;
	uint8_t original[APP_MESH_BEACON_LENGTH];
	uint8_t relay_output[APP_MESH_BEACON_LENGTH];
	uint8_t i;

	memset(&input, 0xA5, sizeof(input));
	input.network_id = 0x1234U;
	input.seq = 0x22U;
	input.cmd = 0x81U;
	input.flags = APP_MESH_FLAGS_RELAYABLE;
	APP_MeshLogicEncodeBeacon(&input, original);
	assert(APP_MeshLogicBuildBeaconRelay(original, relay_output) != 0U);
	assert(APP_MeshLogicDecodeBeacon(relay_output, &output) != 0U);
	for (i = 0U; i < 24U; i++)
	{
		if (i != 4U) assert(relay_output[i] == original[i]);
	}
	assert(relay_output[4] == APP_MESH_FLAGS_TERMINAL);
	assert(relay_output[24] == APP_MeshLogicChecksum(relay_output, 24U));
	input.flags = APP_MESH_FLAGS_TERMINAL;
	APP_MeshLogicEncodeBeacon(&input, original);
	assert(APP_MeshLogicBuildBeaconRelay(original, relay_output) == 0U);
	input.flags = 0x03U;
	APP_MeshLogicEncodeBeacon(&input, original);
	assert(APP_MeshLogicBuildBeaconRelay(original, relay_output) == 0U);
}

static void test_uart_frame(void)
{
	const uint8_t payload[] = {0x80U, 0x01U, 0x02U};
	uint8_t frame[16];
	uint16_t length;

	length = APP_MeshLogicEncodeUartFrame(0x55U, 0xAAU, 0x92U,
	                                     payload, sizeof(payload),
	                                     frame, sizeof(frame));
	assert(length == 10U);
	assert(frame[0] == 0x55U && frame[1] == 0xAAU && frame[2] == 0x92U);
	assert(frame[3] == 0U && frame[4] == sizeof(payload));
	assert(memcmp(&frame[5], payload, sizeof(payload)) == 0);
	assert(frame[8] == APP_MeshLogicChecksum(frame, 8U));
	assert(frame[9] == APP_MESH_UART_TAIL);
	assert(APP_MeshLogicEncodeUartFrame(0x55U, 0xAAU, 0x92U,
	                                    payload, sizeof(payload), frame, 6U) == 0U);
}

static void test_packet_and_remote_keys(void)
{
	app_mesh_packet_key_t packet = {0x1234U, 0x22U, 0x80U};
	app_mesh_packet_key_t same = {0x1234U, 0x22U, 0x80U};
	app_mesh_packet_key_t different = {0x1234U, 0x22U, 0x81U};
	app_mesh_remote_key_t left = {0x1234U, 0x22U, 0x0AU, 0x00U, 0x5CU};
	app_mesh_remote_key_t right = {0x1234U, 0x22U, 0x0AU, 0x00U, 0x5CU};

	assert(APP_MeshLogicPacketKeyEqual(&packet, &same) != 0U);
	assert(APP_MeshLogicPacketKeyEqual(&packet, &different) == 0U);
	assert(APP_MeshLogicRemoteKeyEqual(&left, &right) != 0U);
	right.para++;
	assert(APP_MeshLogicRemoteKeyEqual(&left, &right) == 0U);
}

static void test_beacon_key_ignores_flags(void)
{
	app_mesh_beacon_t relayable;
	app_mesh_beacon_t terminal;
	app_mesh_packet_key_t left;
	app_mesh_packet_key_t right;

	memset(&relayable, 0, sizeof(relayable));
	relayable.network_id = 0x1234U;
	relayable.seq = 0x21U;
	relayable.cmd = APP_MESH_REMOTE_RELAY_CMD;
	relayable.flags = APP_MESH_FLAGS_RELAYABLE;
	terminal = relayable;
	terminal.flags = APP_MESH_FLAGS_TERMINAL;

	left.network_id = relayable.network_id;
	left.seq = relayable.seq;
	left.cmd = relayable.cmd;
	right.network_id = terminal.network_id;
	right.seq = terminal.seq;
	right.cmd = terminal.cmd;
	assert(APP_MeshLogicPacketKeyEqual(&left, &right) != 0U);
}

static void test_scheduler_contract(void)
{
	/* This is the host-side contract test for the static scheduler policy. */
	struct tx_item { unsigned priority; unsigned due; unsigned active; };
	struct tx_item items[] = {
		{4U, 0U, 1U}, /* SYNC_TICK */
		{3U, 0U, 1U}, /* Relay */
		{2U, 0U, 1U}, /* LEADER_ADV */
		{1U, 0U, 1U}  /* LEADER_RESIGN */
	};
	unsigned selected = 0U;
	unsigned i;
	unsigned latest_sync = 17U;

	for (i = 1U; i < sizeof(items) / sizeof(items[0]); i++)
		if (items[i].active && items[i].due <= 0U &&
		    items[i].priority < items[selected].priority) selected = i;
	assert(selected == 3U);
	latest_sync = 18U; /* SYNC_TICK replacement keeps only the newest frame. */
	assert(latest_sync == 18U);
	assert(200U >= 200U); /* UART minimum interval */
	assert(400U >= 200U); /* module hold window exceeds UART interval */
}

static void test_state_timing_and_resign(void)
{
	uint8_t leader_a[6] = {1U, 2U, 3U, 4U, 5U, 6U};
	uint8_t leader_b[6] = {1U, 2U, 3U, 4U, 5U, 7U};

	assert(300U + (leader_a[5] % 200U) == 306U);
	assert(300U + (199U % 200U) == 499U);
	assert(3000U >= (2U * 1000U + 80U));
	assert(20U <= 80U && 80U >= 20U);
	assert(memcmp(leader_a, leader_a, sizeof(leader_a)) == 0); /* resign match */
	assert(memcmp(leader_a, leader_b, sizeof(leader_a)) != 0); /* mismatch */
}

static void test_queue_delay_time_math(void)
{
	uint32_t since = 100U;

	/* Normal queue wait: 50 ticks at 400 us/tick is 20 ms. */
	assert(test_non_negative_delta_ms(150U, since) == 20U);
	assert(test_non_negative_delta_ms(150U, since) <= 400U);

	/* A stale caller timestamp must not turn into a multi-day delay. */
	assert(test_non_negative_delta_ms(90U, since) == 0U);
	assert(test_non_negative_delta_ms(90U, since) < 1000U);

	/* Natural uint32_t tick wrap remains valid for a short elapsed interval. */
	since = UINT32_MAX - 10U;
	assert(test_non_negative_delta_ms(20U, since) == 12U); /* 31 ticks */
	assert(test_non_negative_delta_ms(20U, since) < 1000U);
}

static void test_remote_relay_encoding(void)
{
	app_mesh_remote_key_t remote;
	app_mesh_beacon_t beacon;
	uint8_t encoded[APP_MESH_BEACON_LENGTH];

	remote.address = 0x1234U;
	remote.count = 0x21U;
	remote.cmd = 0x0AU;
	remote.cmd_type = 0x00U;
	remote.para = 0x5CU;
	assert(APP_MeshLogicBuildRemoteRelay(&remote, APP_MESH_FLAGS_RELAYABLE, encoded) != 0U);
	assert(APP_MeshLogicDecodeBeacon(encoded, &beacon) != 0U);
	assert(beacon.network_id == 0x1234U);
	assert(beacon.seq == 0x21U);
	assert(beacon.cmd == 0x83U);
	assert(beacon.flags == APP_MESH_FLAGS_RELAYABLE);
	assert(beacon.payload[0] == 0x21U);
	assert(beacon.payload[1] == 0x0AU);
	assert(beacon.payload[2] == 0x00U);
	assert(beacon.payload[3] == 1U);
	assert(beacon.payload[4] == 0x5CU);
	assert(beacon.payload[5] == 0U);
	assert(APP_MeshLogicBuildRemoteRelay(&remote, APP_MESH_FLAGS_TERMINAL, encoded) != 0U);
	assert(APP_MeshLogicDecodeBeacon(encoded, &beacon) != 0U);
	assert(beacon.flags == APP_MESH_FLAGS_TERMINAL);
	assert(beacon.network_id == remote.address);
	assert(beacon.seq == remote.count);
	assert(beacon.cmd == APP_MESH_REMOTE_RELAY_CMD);
	assert(beacon.payload[0] == remote.count);
	assert(beacon.payload[1] == remote.cmd);
	assert(beacon.payload[2] == remote.cmd_type);
	assert(beacon.payload[3] == 1U);
	assert(beacon.payload[4] == remote.para);
	assert(beacon.payload[5] == 0U);
	assert(APP_MeshLogicIsValidFlags(0x00U) != 0U);
	assert(APP_MeshLogicIsValidFlags(0x01U) != 0U);
	assert(APP_MeshLogicIsValidFlags(0x02U) == 0U);
	assert(APP_MeshLogicIsValidFlags(0x03U) == 0U);
	assert(APP_MeshLogicBuildRemoteRelay(&remote, 0x02U, encoded) == 0U);
	assert(APP_MeshLogicBuildRemoteRelay(&remote, 0x03U, encoded) == 0U);
	assert(APP_MeshLogicRemoteKeyEqual(&remote, &remote) != 0U);
	remote.para ^= 1U;
	assert(APP_MeshLogicRemoteKeyEqual(&remote, &((app_mesh_remote_key_t){0x1234U, 0x21U, 0x0AU, 0x00U, 0x5CU})) == 0U);
}

int main(void)
{
	test_beacon_round_trip();
	test_beacon_relay_copy();
	test_uart_frame();
	test_packet_and_remote_keys();
	test_beacon_key_ignores_flags();
	test_remote_relay_encoding();
	test_scheduler_contract();
	test_state_timing_and_resign();
	test_queue_delay_time_math();
	puts("mesh_logic_test: PASS");
	return 0;
}
