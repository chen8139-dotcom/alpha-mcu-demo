/**
  ******************************************************************************
  * @file    py32f0xx_it.c
  * @author  MCU Application Team
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "py32f0xx_it.h"



#include "user_bsp_uart2.h"
#include "user_bsp_gpio.h"
#include "user_bsp_key.h"




#include "user_common.h"







/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers         */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
}


/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file.                                          */
/******************************************************************************/

/**
  * @brief  This function handles TIM1 interrupt request.
  */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
  if(LL_TIM_IsActiveFlag_UPDATE(TIM1) && LL_TIM_IsEnabledIT_UPDATE(TIM1))
  {
    LL_TIM_ClearFlag_UPDATE(TIM1);
    
    APP_UpdateCallback();
  }
}



/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file.                                          */
/******************************************************************************/

/**
  * @brief This function handles ADC_COMP Interrupt .
  */
void ADC_COMP_IRQHandler(void)
{
  /* Detect if the interrupt is triggered by the end of conversion */
  if(LL_ADC_IsActiveFlag_EOC(ADC1) != 0)
  {
    /* Clear ADC EOC interrupt */
    LL_ADC_ClearFlag_EOC(ADC1);


  }
}

/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  if (LL_DMA_IsActiveFlag_TC1(DMA1) != 0U)
  {
    LL_DMA_ClearFlag_TC1(DMA1);
    
  }
}




/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file.                                          */
/******************************************************************************/
/**
  * @brief  This function handles key EXTI interrupt.
  */



#if DEF_SOP8_SOP16

void EXTI0_1_IRQHandler(void)
{




  /* Handling EXTI interrupt requests */
  if(LL_EXTI_IsActiveFlag(LL_EXTI_LINE_1))
  {
    //BSP_LED_Toggle(LED_GREEN);
    LL_EXTI_ClearFlag(LL_EXTI_LINE_1);
	 
	APP_KeyCallback();
	
  }



}

#else

/**
  * @brief  This function handles EXTI4_15 interrupt.
  */
void EXTI4_15_IRQHandler(void)
{
#if DEF_SOP20_PACKAGE
  if (LL_EXTI_IsActiveFlag(LL_EXTI_LINE_12))
  {
    LL_EXTI_ClearFlag(LL_EXTI_LINE_12);
    APP_KeyCallback();
  }
#else
  if (LL_EXTI_IsActiveFlag(LL_EXTI_LINE_5))
  {
    LL_EXTI_ClearFlag(LL_EXTI_LINE_5);
    APP_KeyCallback();
  }
#endif
}
#endif






/**
  * @brief This function handles USART1 interrupt.
  */
void USART1_IRQHandler(void)
{   
  APP_UsartIRQCallback(USART1);
}




/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
