

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
#include "user_bsp_uart1.h"
#include "user_bsp_dma.h"

#include "py32f0xx_ll_usart.h"

#include <string.h>
#include <stdint.h>

#include "user_board_cfg.h"


/**
  ******************************************************************************
  * @file    main.c
  * @author  MCU Application Team
  * @brief   Main program body
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
//#include "main.h"
//#include "py32f003xx_ll_Start_Kit.h"
//#include <string.h>


#include "user_bsp_uart1.h"

extern volatile uint16_t g_Usart1FrameLen;
extern volatile unsigned char g_Usart1FrameReady;

extern unsigned char  g_Usart1RxDmaBuf[USART1_RXBUFF_SIZE];
extern unsigned char  g_Usart1FrameBuf[USART1_RXBUFF_SIZE];




/* UART1 receive ISR must not perform logging or blocking TX. */




static void APP_UsartRxIdleCallback(USART_TypeDef *USARTx);





/**
  * @brief  USART configuration
  * @param  USARTx：USART module, can be USART1 or USART2
  * @retval None
  */
void APP_ConfigUsart(USART_TypeDef *USARTx)
{
  /*Enable clock, initialize pins, enable NVIC interrupt*/
  if (USARTx == USART1) 
  {
    /*Enable GPIOA clock*/
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    /*Enable USART1 clock*/
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
        
    /*GPIOA configuration*/
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    /*Select pin 2*/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    /*Select alternate function mode*/
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    /*Select output speed*/
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    /*Select push-pull output mode*/
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    /*Select pull-up*/
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    /*Select USART1 function*/
    GPIO_InitStruct.Alternate = LL_GPIO_AF1_USART1;
    /*Initialize GPIOA*/
    LL_GPIO_Init(GPIOA,&GPIO_InitStruct);
    



    /** U1_RX 配置 */



	
	/*Select pin 3*/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    /*Select USART1 function*/
    GPIO_InitStruct.Alternate = LL_GPIO_AF1_USART1;

	
    /*Initialize GPIOA*/
    LL_GPIO_Init(GPIOA,&GPIO_InitStruct);


	/**
	  * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	 */
	
	// PA13
	//LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_13, LL_GPIO_MODE_ANALOG);

	





	
    
    /*Set USART1 interrupt priority*/
    NVIC_SetPriority(USART1_IRQn,0);
    /*Enable USART1 interrupt*/
    NVIC_EnableIRQ(USART1_IRQn);
  }

  

  /* Configure DMA */
  APP_Uart1_Dma_Init(USARTx);

  /*USART configuration*/
  LL_USART_InitTypeDef USART_InitStruct = {0};
  /* BLE 模组与厂家文档：115200 8N1；此前 9600 会导致收包全错、无法解析协议帧 */
  USART_InitStruct.BaudRate = 115200U;
  /*Set data width*/
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  /* Set stop bits */
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  /* Set parity */
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  /*Initialize USART*/
  LL_USART_Init(USARTx, &USART_InitStruct);
  
  /*Configure as full duplex asynchronous mode*/
  LL_USART_ConfigAsyncMode(USARTx);
  
  /*Enable UART module*/
  LL_USART_Enable(USARTx);
}



/**
  * @brief  USART transmit function
  * @param  USARTx：USART module, can be USART1 or USART2
  * @param  pData：transmit buffer
  * @param  Size：Size of the transmit buffer
  * @retval None
  */
void APP_UsartTransmit(USART_TypeDef *USARTx, const uint8_t *pData, uint16_t Size)
{  
  uint16_t i;
  if (Size == 0U)
  {
    return;
  }
  
  for (i = 0U; i < Size; i++)
  {
    while (LL_USART_IsActiveFlag_TXE(USARTx) == RESET) {}
    LL_USART_TransmitData8(USARTx, pData[i]);
  }
  
  while (LL_USART_IsActiveFlag_TC(USARTx) == RESET) {}
}

/**
  * @brief  USART receive function
  * @param  USARTx：USART module, can be USART1 or USART2
  * @param  pData：receive buffer
  * @param  Size：Size of the receive buffer
  * @retval None
  */
void APP_UsartReceive_DMA(USART_TypeDef *USARTx, uint8_t *pData, uint16_t Size)
{
  if (Size == 0U)
  {
    return;
  }

  /*Configure DMA channel2*/
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_ClearFlag_GI2(DMA1);
  uint32_t *temp = (uint32_t *)&pData;
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, *(uint32_t *)temp);
  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&USARTx->DR);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, Size);
  LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
  
  LL_USART_ClearFlag_ORE(USARTx);
  LL_USART_ClearFlag_IDLE(USARTx);
  
  /* Enable RX DMA request + IDLE interrupt */
  LL_USART_EnableDMAReq_RX(USARTx);
  LL_USART_EnableIT_IDLE(USARTx);
}

/**
  * @brief  USART interrupt callback function
  * @param  USARTx：USART module, can be USART1 or USART2
  * @retval None
  */
void APP_UsartIRQCallback(USART_TypeDef *USARTx)
{
  /* IDLE line frame event */
  if ((LL_USART_IsActiveFlag_IDLE(USARTx) != RESET) && (LL_USART_IsEnabledIT_IDLE(USARTx) != RESET))
  {
    APP_UsartRxIdleCallback(USARTx);
  }

}



/**
  * @brief  USART RX IDLE callback, latch one frame from DMA circular buffer
  * @param  USARTx：USART module, can be USART1 or USART2
  * @retval None
  */
static void APP_UsartRxIdleCallback(USART_TypeDef *USARTx)
{
  uint16_t dma_remaining;
  uint16_t frame_len;
  volatile uint32_t tmp;

  /* Clear IDLE flag: read SR then DR */
  tmp = USARTx->SR;
  tmp = USARTx->DR;
  (void)tmp;

  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
  dma_remaining = (uint16_t)LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_2);
  frame_len = (uint16_t)(USART1_RXBUFF_SIZE - dma_remaining);

  if (frame_len > USART1_RXBUFF_SIZE)
  {
    frame_len = USART1_RXBUFF_SIZE;
  }

  if (frame_len > 0U)
  {
    /* The ISR only latches the frame. Parsing, TX and logging stay in main. */
    if (g_Usart1FrameReady == 0U)
    {
      memcpy((void *)g_Usart1FrameBuf, (const void *)g_Usart1RxDmaBuf, frame_len);


	  /////////////









	  ////////////////////


	  
      g_Usart1FrameLen = frame_len;
      g_Usart1FrameReady = 1U;
    }
  }

  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, USART1_RXBUFF_SIZE);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}












