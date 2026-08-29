/**
  ******************************************************************************
  * @file    user_ble_uart.h
  * @brief   UART1 Beacon Mesh protocol and MCU leader runtime.
  ******************************************************************************
  */

#ifndef USER_BLE_UART_H
#define USER_BLE_UART_H

#include <stdint.h>

void APP_BleMeshInit(void);
void APP_HandleBleUartFrame(void);
void APP_BleMeshSetSyncMode(uint8_t dynamic);
void APP_BleMeshNotifySyncStateChanged(void);
void APP_BleMeshRequestResign(uint8_t reason);

#endif /* USER_BLE_UART_H */
