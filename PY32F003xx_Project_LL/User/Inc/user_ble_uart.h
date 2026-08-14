/**
  ******************************************************************************
  * @file    user_ble_uart.h
  * @brief   UART1 一帧分发：配网（user_ble_pair）+ APP 开关灯
  ******************************************************************************
  */

#ifndef USER_BLE_UART_H
#define USER_BLE_UART_H

/**
  * @brief  处理 UART1 IDLE 切出的一帧（配网 AA55/55AA、APP 开关 55AA）
  */
void APP_HandleBleUartFrame(void);

#endif /* USER_BLE_UART_H */
