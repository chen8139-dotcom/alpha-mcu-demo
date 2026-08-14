

///////////////////////////


/**
  ******************************************************************************
  * @file    py32f003xx_ll_Start_Kit.c
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

/* Includes ------------------------------------------------------------------*/
#include "user_bsp_dma.h"
#include "user_bsp_adc.h"

/* Includes ------------------------------------------------------------------*/
#include "main.h"
//#include "py32f003xx_ll_Start_Kit.h"
#include <string.h>





void APP_Dma_Init(void)
{


	/* Enable DMA1 clock */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	



}













/**
  * @brief  DMA configuration function
  * @param  USARTx：USART module, can be USART1 or USART2
  * @retval None
  */
void APP_Uart1_Dma_Init(USART_TypeDef *USARTx) 
{

  
 /* Configure DMA channel LL_DMA_CHANNEL_2 for reception */
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY | \
                      LL_DMA_MODE_CIRCULAR                 | \
                      LL_DMA_PERIPH_NOINCREMENT  | \
                      LL_DMA_MEMORY_INCREMENT  | \
                      LL_DMA_PDATAALIGN_BYTE | \
                      LL_DMA_MDATAALIGN_BYTE | \
                      LL_DMA_PRIORITY_LOW);

    /* Configure DMA request mapping for channels associated with USART1 and USART2 */
  if (USARTx ==  USART1)
  {
    /* USART1_RX maps to channel LL_DMA_CHANNEL_2 */
    LL_SYSCFG_SetDMARemap_CH2(LL_SYSCFG_DMA_MAP_USART1_RX);
  }
  else
  {
    /* USART2_RX maps to channel LL_DMA_CHANNEL_2 */
    LL_SYSCFG_SetDMARemap_CH2(LL_SYSCFG_DMA_MAP_USART2_RX);
  }    
  
  /*Set interrupt priority*/
  NVIC_SetPriority(DMA1_Channel2_3_IRQn, 1);
  /*Enable interrupt*/
  NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}




