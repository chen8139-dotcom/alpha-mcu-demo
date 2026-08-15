

////////////////////////////

/**
  ******************************************************************************
  * @file    py32f003xx_ll_Start_Kit.h
  * @author  MCU Application Team
  * @brief   This file provides set of firmware functions to manage Leds, 
  *          push-button available on Start Kit.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by Puya under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BSP_UART2_H
#define __USER_BSP_UART2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_system.h"
#include "py32f0xx_ll_exti.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_pwr.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_usart.h"


#include "user_board_cfg.h"


/** @addtogroup BSP
  * @{
  */

/** @defgroup py32f0xx_Start_Kit
  * @brief This section contains the exported types, contants and functions
  *        required to use the Nucleo 32 board.
  * @{
  */

/** @defgroup py32f0xx_Start_Kit_Exported_Types Exported Types
  * @{
  */





#define StartKitVersion 2
/* #define StartKitVersion 1 */







#ifdef USART2
//debug printf redirect config
#define DEBUG_USART_BAUDRATE                    115200

#define DEBUG_USART                             USART2
#define DEBUG_USART_CLK_ENABLE()                LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2)

#define __GPIOB_CLK_ENABLE()                    do { \
                                                     __IO uint32_t tmpreg = 0x00U; \
                                                     SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN);\
                                                     /* Delay after an RCC peripheral clock enabling */ \
                                                     tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN);\
                                                     UNUSED(tmpreg); \
                                                   } while(0U)

#define DEBUG_USART_RX_GPIO_PORT                GPIOB
#define DEBUG_USART_RX_GPIO_CLK_ENABLE()        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB)
#define DEBUG_USART_RX_PIN                      LL_GPIO_PIN_7
#define DEBUG_USART_RX_AF                       LL_GPIO_AF_4

#define DEBUG_USART_TX_GPIO_PORT                GPIOB
#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB)
#define DEBUG_USART_TX_PIN                      LL_GPIO_PIN_6
#define DEBUG_USART_TX_AF                       LL_GPIO_AF_4

#define DEBUG_USART_IRQHandler                  USART2_IRQHandler
#define DEBUG_USART_IRQ                         USART2_IRQn
#else
#define DEBUG_USART_TX_GPIO_PORT                GPIOA
#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA)
#define DEBUG_USART_TX_PIN                      LL_GPIO_PIN_2
#endif
/************************************************************/

/** @defgroup Functions
  * @{
  */
uint32_t         BSP_GetVersion(void);





void             BSP_USART_Config(void);

void print_hex_array(const unsigned char *buf, unsigned short len);





/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USER_BSP_UART2_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE****/










