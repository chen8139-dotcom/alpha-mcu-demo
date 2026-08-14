/**
  ******************************************************************************
  * @file    user_ble_pair.c
  * @brief   BLE 模组与普冉 MCU 配网 UART 协议（V1.3 + 产品层 0x21）
  *
  * 校验：从首字节到校验字节前一字节无符号累加，取低 8 位，与倒数第二字节比较。
  ******************************************************************************
  */

#include "user_ble_pair.h"

#include <stdio.h>
#include <string.h>

#include "py32f0xx_ll_usart.h"
#include "user_bsp_uart1.h"
#include "user_common.h"
#include "user_flash_manage.h"
#include "user_light_pattern.h"




