



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BSP_KEY_H
#define __USER_BSP_KEY_H


//#include "py32f0xx.h"   // 包含GPIO相关的寄存器定义，例如 GPIOB->BSRR 使用
#include "py32f0xx_ll_gpio.h"
#include "user_board_cfg.h"


#ifndef DEF_SOP8_SOP16
#error "DEF_SOP8_SOP16 is not defined. Please include user_board_cfg.h first."
#endif


#if DEF_SOP8_SOP16 /* 1:SOP16(PF1) */
#define KE3 (LL_GPIO_IsInputPinSet(GPIOF, LL_GPIO_PIN_1))
#elif DEF_SOP20_PACKAGE /* 1:SOP20(PA12); 0:SOP8(PB5) */
#define KE3 (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_12))
#else
#define KE3 (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_5))
#endif



//GPIOB->BSRR = (1U << 0); 

//GPIOB->BSRR = (1U << (0 + 16));


extern uint8_t G_Var_Evt_evt_key_short;
extern uint8_t G_Var_Evt_evt_key_long;



void APP_ConfigureExti(void);

void APP_KeyCallback(void);

void KeyScan(void);




#endif /* __USER_BSP_KEY_H */




