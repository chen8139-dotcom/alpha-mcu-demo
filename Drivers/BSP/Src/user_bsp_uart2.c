

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
#include "user_bsp_uart2.h"

/**
  * @brief PY32F003xx STK BSP Driver version number
  */
#define __PY32F003xx_STK_BSP_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define __PY32F003xx_STK_BSP_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define __PY32F003xx_STK_BSP_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define __PY32F003xx_STK_BSP_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __PY32F003xx_STK_BSP_VERSION        ((__PY32F003xx_STK_BSP_VERSION_MAIN << 24) \
                                            |(__PY32F003xx_STK_BSP_VERSION_SUB1 << 16) \
                                            |(__PY32F003xx_STK_BSP_VERSION_SUB2 << 8 ) \
                                            |(__PY32F003xx_STK_BSP_VERSION_RC))





/** @addtogroup PY32F003xx_STK_Exported_Functions
  * @{
  */

/**
  * @brief  This method returns the PY32F003 STK BSP Driver revision.
  * @retval version : 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t BSP_GetVersion(void)
{
  return __PY32F003xx_STK_BSP_VERSION;
}



void print_hex_array(const uint8_t *buf, uint16_t len)
{
	printf("\r\n");

    for (uint16_t i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);   // 两位16进制，大写，不足补0
    }
    printf("\r\n");
	printf("\r\n");
}


#ifdef USART2
/**
  * @brief  DEBUG_USART GPIO Config,Mode Config,115200 8-N-1
  * @param  None
  * @retval None
  */
void BSP_USART_Config(void)
{
#if  defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0 );
#endif

  DEBUG_USART_CLK_ENABLE();

  /* USART Init */
  LL_USART_SetBaudRate(DEBUG_USART, SystemCoreClock, LL_USART_OVERSAMPLING_16, DEBUG_USART_BAUDRATE);
  LL_USART_SetDataWidth(DEBUG_USART, LL_USART_DATAWIDTH_8B);
  LL_USART_SetStopBitsLength(DEBUG_USART, LL_USART_STOPBITS_1);
  LL_USART_SetParity(DEBUG_USART, LL_USART_PARITY_NONE);
  LL_USART_SetHWFlowCtrl(DEBUG_USART, LL_USART_HWCONTROL_NONE);
  LL_USART_SetTransferDirection(DEBUG_USART, LL_USART_DIRECTION_TX_RX);
  LL_USART_Enable(DEBUG_USART);
  LL_USART_ClearFlag_TC(DEBUG_USART);

  /**USART GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
  DEBUG_USART_RX_GPIO_CLK_ENABLE();
  DEBUG_USART_TX_GPIO_CLK_ENABLE();

  LL_GPIO_SetPinMode(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  LL_GPIO_SetPinPull(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_PULL_UP);
  LL_GPIO_SetAFPin_0_7(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, DEBUG_USART_TX_AF);

  LL_GPIO_SetPinMode(DEBUG_USART_RX_GPIO_PORT, DEBUG_USART_RX_PIN, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(DEBUG_USART_RX_GPIO_PORT, DEBUG_USART_RX_PIN, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  LL_GPIO_SetPinPull(DEBUG_USART_RX_GPIO_PORT, DEBUG_USART_RX_PIN, LL_GPIO_PULL_UP);
  LL_GPIO_SetAFPin_0_7(DEBUG_USART_RX_GPIO_PORT, DEBUG_USART_RX_PIN, DEBUG_USART_RX_AF);
}

#if (defined (__CC_ARM)) || (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
/**
  * @brief  writes a character to the usart
  * @param  ch
  *         *f
  * @retval the character
  */
int fputc(int ch, FILE *f)
{
  /* Send a byte to USART */
  LL_USART_TransmitData8(DEBUG_USART, ch);
  while (!LL_USART_IsActiveFlag_TC(DEBUG_USART));
  LL_USART_ClearFlag_TC(DEBUG_USART);

  return (ch);
}

/**
  * @brief  get a character from the usart
  * @param  *f
  * @retval a character
  */
int fgetc(FILE *f)
{
  int ch;
  while (!LL_USART_IsActiveFlag_RXNE(DEBUG_USART));
  ch = LL_USART_ReceiveData8(DEBUG_USART);
  return (ch);
}

#elif defined(__ICCARM__)
/**
  * @brief  writes a character to the usart
  * @param  ch
  *         *f
  * @retval the character
  */
int putchar(int ch)
{
  /* Send a byte to USART */
  LL_USART_TransmitData8(DEBUG_USART, ch);
  while (!LL_USART_IsActiveFlag_TC(DEBUG_USART));
  LL_USART_ClearFlag_TC(DEBUG_USART);

  return (ch);
}
#elif  defined(__GNUC__)
/**
  * @brief  writes a character to the usart
  * @param  ch
  * @retval the character
  */
int __io_putchar(int ch)
{
  /* Send a byte to USART */
  LL_USART_TransmitData8(DEBUG_USART, ch);
  while (!LL_USART_IsActiveFlag_TC(DEBUG_USART));
  LL_USART_ClearFlag_TC(DEBUG_USART);
  return ch;
}

int _write(int file, char *ptr, int len)
{
  int DataIdx;
  for (DataIdx=0;DataIdx<len;DataIdx++)
  {
    __io_putchar(*ptr++);
  }
  return len;
}

#endif
#else
static uint32_t ReloadD = 0;
static uint32_t MinD = 0;

/**
  * @brief  GPIO Config As USART
  * @param  None
  * @retval None
  */
void BSP_USART_Config(void)
{

  DEBUG_USART_TX_GPIO_CLK_ENABLE();

  LL_GPIO_SetPinMode(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  LL_GPIO_SetPinPull(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN, LL_GPIO_PULL_UP);

  ReloadD = SysTick->LOAD;
  MinD = (ReloadD+1)*10/96;
}

/**
  * @brief  writes a character to the usart
  * @param  SystickStart
  * @param  ReloadData  
  * @param  MinData
  * @retval NULL
  */
static void BSP_Delay(uint32_t SystickStart,uint32_t ReloadData, uint32_t MinData)
{
   uint32_t SystickCurrent = 0;
   while(1)
   {     
     SystickCurrent = SysTick->VAL;

     if(SystickCurrent < SystickStart)
     {
       if((SystickStart-SystickCurrent) >= MinData)
       {
         break;
       }
     }
     else
     {
       if(SystickCurrent <= (ReloadData+1 - MinData + SystickStart))
       {
         break;
       }
     }
   }
}

/**
  * @brief  writes a character to the usart
  * @param  ch
  *         *f
  * @retval the character
  */
static void BSP_TransmitChar(uint8_t ch)
{
  uint8_t temp = 0;
  uint32_t SystickStart = SysTick->VAL ;

  for(uint8_t i=0;i<10;i++)
  {
    if(i==0)
    {
      LL_GPIO_ResetOutputPin(DEBUG_USART_TX_GPIO_PORT,DEBUG_USART_TX_PIN);
    }
    else if(i == 9)
    { 
      BSP_Delay(SystickStart, ReloadD, MinD);

      LL_GPIO_SetOutputPin(DEBUG_USART_TX_GPIO_PORT,DEBUG_USART_TX_PIN);
  
      if(SystickStart >= MinD)
      {
        SystickStart = SystickStart - MinD;
      }
      else
      {
        SystickStart = ReloadD+1 + SystickStart - MinD;
      }
      BSP_Delay(SystickStart, ReloadD, MinD);
    }
    else
    {
      temp=ch&0x01;
      BSP_Delay(SystickStart, ReloadD, MinD); 
      switch(temp)
      {
        case 1: 
        {
          LL_GPIO_SetOutputPin(DEBUG_USART_TX_GPIO_PORT,DEBUG_USART_TX_PIN);
          break;
        }
        case 0:
        {
          LL_GPIO_ResetOutputPin(DEBUG_USART_TX_GPIO_PORT,DEBUG_USART_TX_PIN);
          break;
        }

        default: break;
      }
      ch>>=1;
      if(SystickStart > MinD)
      {
        SystickStart = SystickStart - MinD;
      }
      else
      {
        SystickStart = ReloadD + 1 + SystickStart - MinD;
      }
    }
  }
}

#if (defined (__CC_ARM)) || (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
/**
  * @brief  writes a character to the usart
  * @param  ch
  *         *f
  * @retval the character
  */
int fputc(int ch, FILE *f)
{
  BSP_TransmitChar(ch);
  return (ch);
}


#elif defined(__ICCARM__)
/**
  * @brief  writes a character to the usart
  * @param  ch
  *         *f
  * @retval the character
  */
int putchar(int ch)
{
  /* Send a byte to USART */
  BSP_TransmitChar(ch);

  return (ch);
}
#elif  defined(__GNUC__)
/**
  * @brief  writes a character to the usart
  * @param  ch
  * @retval the character
  */
int __io_putchar(int ch)
{
  /* Send a byte to USART */
  BSP_TransmitChar(ch);

  return ch;
}

int _write(int file, char *ptr, int len)
{
  int DataIdx;
  for (DataIdx=0;DataIdx<len;DataIdx++)
  {
    __io_putchar(*ptr++);
  }
  return len;
}
#endif











#endif

//#endif
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT Puya *****END OF FILE****/









