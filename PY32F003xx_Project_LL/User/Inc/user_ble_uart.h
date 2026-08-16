/**
  ******************************************************************************
  * @file    user_ble_uart.h
  * @brief   UART1 Beacon Mesh protocol and MCU leader runtime.
  ******************************************************************************
  */

#ifndef USER_BLE_UART_H
#define USER_BLE_UART_H

void APP_BleMeshInit(void);
void APP_HandleBleUartFrame(void);

#endif /* USER_BLE_UART_H */
