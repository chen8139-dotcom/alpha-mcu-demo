/**
  ******************************************************************************
  * @file    user_ble_uart.c
  * @brief   UART1 BLE/模组帧解析与分发（配网 + APP 开关）
  ******************************************************************************
  */

#include "user_ble_uart.h"

#include <stdint.h>
#include <stdio.h>

#include "user_ble_pair.h"
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

static uint8_t APP_BleFrameChecksum(const uint8_t *p, uint16_t byte_count)
{
	uint32_t s = 0U;
	uint16_t i;

	for (i = 0U; i < byte_count; i++)
	{
		s += (uint32_t)p[i];
	}
	return (uint8_t)(s & 0xFFU);
}

/**
  * @brief  APP 下发开关灯帧处理。命中返回 1（含校验错/动作非法），未命中返回 0。
  */
static uint8_t APP_HandleBleSwitchFrame(const uint8_t *buf, uint16_t frame_len)
{
	/* APP 开关灯：55 AA 00 04 02 01 01 XX CHK FE，len=10；XX=00 开灯，XX=01 关灯 */
	if (frame_len != 10U ||
	    buf[0] != 0x55U || buf[1] != 0xAAU || buf[2] != 0x00U || buf[3] != 0x04U ||
	    buf[4] != 0x02U || buf[5] != 0x01U || buf[6] != 0x01U)
	{
		return 0U;
	}

	if (APP_BleFrameChecksum(buf, 8U) != buf[8])
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[BLE] App switch: checksum error.\r\n");
#endif
		return 1U;
	}

	if (buf[7] == 0x00U)
	{
		WS_SWITCH_H;
		Delay_ms(5);
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[BLE] App light: ON (WS_SWITCH_H)\r\n");
#endif
		return 1U;
	}

	if (buf[7] == 0x01U)
	{
		WS_SWITCH_L;
		Delay_ms(5);
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[BLE] App light: OFF (WS_SWITCH_L)\r\n");
#endif
		return 1U;
	}

#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[BLE] App switch: invalid action 0x%02X.\r\n", (unsigned int)buf[7]);
#endif
	return 1U;
}

/**
  * @brief  UART1 一帧处理：先配网（user_ble_pair），再 APP 开关灯。
  */
void APP_HandleBleUartFrame(void)
{
	uint16_t frame_len;
	uint16_t i;
	uint16_t start_idx = 0U;
	uint16_t end_idx = 0U;
	uint8_t frame_aligned = 0U;
	uint8_t tail_found = 0U;
	uint8_t *buf;

	if (g_Usart1FrameReady == 0U)
	{
		return;
	}

	DISI();

	frame_len = g_Usart1FrameLen;
	if (frame_len > USART1_RXBUFF_SIZE)
	{
		frame_len = USART1_RXBUFF_SIZE;
	}

	buf = g_Usart1FrameBuf;

#if APP_BLE_DEBUG_LOG && APP_VERBOSE_UART1_RX
	printf("[UART1 RX] len=%u ", (unsigned int)frame_len);
	print_hex_array((const uint8_t *)buf, frame_len);
#endif

	if (frame_len < 2U)
	{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
		printf("[UART1 RX] invalid frame: too short len=%u\r\n", (unsigned int)frame_len);
#endif
		goto APP_HandleBleUartFrame_Exit;
	}

	if (buf[frame_len - 1U] != 0xFEU)
	{
		for (i = frame_len; i > 0U; i--)
		{
			if (buf[i - 1U] == 0xFEU)
			{
				end_idx = (uint16_t)(i - 1U);
				tail_found = 1U;
				break;
			}
		}
		if (tail_found != 0U)
		{
			frame_len = (uint16_t)(end_idx + 1U);
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
			printf("[UART1 RX] FE realign to idx=%u len=%u\r\n",
			       (unsigned int)end_idx, (unsigned int)frame_len);
#endif
		}
		else
		{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
			printf("[UART1 RX] invalid frame: no FE in buffer, len=%u\r\n",
			       (unsigned int)frame_len);
#endif
			goto APP_HandleBleUartFrame_Exit;
		}
	}

	/* 帧头重定位：AA55(模组→MCU 配网) 或 55AA(APP/业务) */
	for (i = 0U; i + 1U < frame_len; i++)
	{
		if ((buf[i] == 0xAAU && buf[i + 1U] == 0x55U) ||
		    (buf[i] == 0x55U && buf[i + 1U] == 0xAAU))
		{
			start_idx = i;
			frame_aligned = 1U;
			break;
		}
	}
	if (frame_aligned != 0U && start_idx > 0U)
	{
		frame_len = (uint16_t)(frame_len - start_idx);
		buf = &buf[start_idx];
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG && APP_VERBOSE_UART1_RX
		printf("[UART1 RX] realign to AA55/55AA at +%u, len=%u\r\n",
		       (unsigned int)start_idx, (unsigned int)frame_len);
#endif
	}



	if (APP_HandleBleSwitchFrame(buf, frame_len) != 0U)
	{
		goto APP_HandleBleUartFrame_Exit;
	}

#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	printf("[BLE] Unknown command frame (not pair/switch).\r\n");
#endif

APP_HandleBleUartFrame_Exit:
	g_Usart1FrameReady = 0U;
	ENI();
}
