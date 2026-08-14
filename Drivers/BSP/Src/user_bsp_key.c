


#include <stdio.h>

#include "py32f0xx_ll_exti.h"
#include "py32f0xx_ll_bus.h"

#include "user_bsp_key.h"

#include "user_bsp_type.h"

#include "user_board_cfg.h"


#include "user_common.h"

//////////////////////////////////////////////////////////////

uint8_t G_Var_Key_key_state;
uint16_t G_Var_Key_key_press_time;			   // 按键按下时长（KeyScan 每 800us +1）
uint8_t G_Var_Evt_evt_key_short;		   // 短按事件（松开且 <3s）
uint8_t G_Var_Evt_evt_key_long;			   // 长按事件（按住满 3s）

/* KeyScan 在 Task_800us 调用，周期 800us */
#define KEY_SCAN_PERIOD_US     800U
#define KEY_LONG_PRESS_MS      3000U
#define KEY_LONG_PRESS_TICKS   ((KEY_LONG_PRESS_MS * 1000U + KEY_SCAN_PERIOD_US - 1U) / KEY_SCAN_PERIOD_US)

static uint8_t G_Var_Key_long_fired;		   // 本次按下已触发长按，防重复置位



void KeyScan(void)
{

	switch(G_Var_Key_key_state)
	{
		case 0: // 空闲状态

			if (G_Var_Key_key_fall_flag)
			{
				G_Var_Key_key_fall_flag = 0;

				// 如果KE3按键按下（低电平有效）- 模式切换键
				if(!KE3) // 检测到按下（按照流程图：KE3按键按下）
				{
					G_Var_Key_key_state = 1;
				 	G_Var_Key_key_press_time = 0;
					G_Var_Key_long_fired = 0;
				}

			}
			
			break;

		case 1: // 按下消抖
			if(!KE3) // 检测到按下（按照流程图：KE3按键按下）
			{
				G_Var_Key_key_press_time++;
				if (G_Var_Key_key_press_time >= 10)  // 10ms消抖
				{
					G_Var_Key_key_state = 2;  // 确认按下
				}
			}
			else
			{
				G_Var_Key_key_state = 0;  // 误触发，返回空闲
			}
			
			break;

		case 2: // 按下保持
            if (!KE3)
            {
                G_Var_Key_key_press_time++;
                if (!G_Var_Key_long_fired &&
                    G_Var_Key_key_press_time >= KEY_LONG_PRESS_TICKS)
                {
                    G_Var_Key_long_fired = 1;
                    G_Var_Evt_evt_key_long = 1;
                }
            }
            else  // 按键释放
            {
                if (!G_Var_Key_long_fired &&
                    G_Var_Key_key_press_time < KEY_LONG_PRESS_TICKS)
                {
                    G_Var_Evt_evt_key_short = 1;
                }

                G_Var_Key_long_fired = 0;
                G_Var_Key_key_state = 0;
                G_Var_Key_key_press_time = 0;
            }

			break;

		default:
			G_Var_Key_key_state = 0;
			break;

	}


}









//////////////////////////////////////////////////////////



#if DEF_SOP8_SOP16 //[1:SOP16]

void APP_ConfigureExti(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};

  /* Configure key pin as input mode */

  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;

  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOF, &GPIO_InitStruct);


  /* Configure EXTI as interrupt, falling edge triggered */
  EXTI_InitStruct.Line = LL_EXTI_LINE_1;

  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /* When using EXTI channels 0~8, configure the trigger port, for example: */
  /* LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA,LL_EXTI_CONFIG_LINE8); */


  LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE1);


  /* Enable interrupt */
  NVIC_SetPriority(EXTI0_1_IRQn, 0);
  NVIC_EnableIRQ(EXTI0_1_IRQn);


  /**
   * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
  */
	
  // PA14
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);

  
}

#else //[0: SOP8]

#if !DEF_SOP20_PACKAGE //[SOP8 PB5]
void APP_ConfigureExti(void)
{

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};


	  /**
	   * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	  */
		
	  // PA14
	  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);

	  
	
	  /* Configure key pin as input mode */
	
	  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
	
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	
	  /* Configure EXTI as interrupt, falling edge triggered */
	  EXTI_InitStruct.Line = LL_EXTI_LINE_5;
	
	  EXTI_InitStruct.LineCommand = ENABLE;
	  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
	  LL_EXTI_Init(&EXTI_InitStruct);
	
	  /* When using EXTI channels 0~8, configure the trigger port, for example: */
	  /* LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA,LL_EXTI_CONFIG_LINE8); */
	
	
	  LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE5);
	
	
	  /* Enable interrupt */
	  NVIC_SetPriority(EXTI4_15_IRQn, 0);
	  NVIC_EnableIRQ(EXTI4_15_IRQn);
	
	





}

#endif


#if DEF_SOP20_PACKAGE //[SOP20 PA12]
void APP_ConfigureExti(void)
{

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};


	  /**
	   * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	  */
		
	  // PA14
	  //LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);

	  
	
	  /* Configure key pin as input mode */
	
	  GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
	
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	  /* Configure EXTI as interrupt, falling edge triggered */
	  EXTI_InitStruct.Line = LL_EXTI_LINE_12;
	
	  EXTI_InitStruct.LineCommand = ENABLE;
	  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
	  LL_EXTI_Init(&EXTI_InitStruct);
	
	  /* EXTI9~15 固定映射 PA9~PA15，无需 SetEXTISource（仅 EXTI0~8 可配端口） */
	
	
	  /* Enable interrupt */
	  NVIC_SetPriority(EXTI4_15_IRQn, 0);
	  NVIC_EnableIRQ(EXTI4_15_IRQn);
	
	





}

#endif





#endif


void APP_KeyCallback(void)
{

	// 如果KE3按键按下（低电平有效）- 模式切换键
	if(!KE3)
	{
		
		// 清除启动标志
		//qden=0;
		// 设置KE3按键按下标志
		G_Var_Key_key_fall_flag=1;
		// 记录按下时的时间戳（用于防抖）
		stap1=wos;

		

    }


}




