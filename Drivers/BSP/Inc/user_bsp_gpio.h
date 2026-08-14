



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BSP_GPIO_H
#define __USER_BSP_GPIO_H

//#include "py32f0xx.h"   // 包含GPIO相关的寄存器定义，例如 GPIOB->BSRR 使用
#include "py32f0xx_ll_gpio.h"

/* 原九齐 KE3：头键口线读值。本板 PB1 内部上拉（见 user_key.c），松开=高=1、按下=低=0 */
//#define KE3  (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_1) != 0U)







#include "user_board_cfg.h"

#ifndef DEF_SOP8_SOP16
#error "DEF_SOP8_SOP16 is not defined. Please include user_board_cfg.h first."
#endif




//GPIOB->BSRR = (1U << 0); 

//GPIOB->BSRR = (1U << (0 + 16));







#if DEF_SOP8_SOP16 //[1:SOP16]

#define WS_IO_H       (GPIOF->BSRR = (1U << 0))
#define WS_IO_L       (GPIOF->BSRR = (1U << (0 + 16)))



#else  //[0:SOP8]



/* PB0 */
//#define WS_IO_H       (GPIOB->BSRR = (1U << 0))
//#define WS_IO_L       (GPIOB->BSRR = (1U << (0 + 16)))


/* PB6 */
#define WS_IO_H       (GPIOB->BSRR = (1U << 6))
#define WS_IO_L       (GPIOB->BSRR = (1U << (6 + 16)))




#endif





#define WS_SWITCH_H       (GPIOA->BSRR = (1U << 1))
#define WS_SWITCH_L       (GPIOA->BSRR = (1U << (1 + 16)))



#define LEDB_H       (GPIOA->BSRR = (1U << 4)) //关灯
#define LEDB_L       (GPIOA->BSRR = (1U << (4 + 16)))   //开灯






void APP_Gpio_Clock_Init(void);

void APP_GpioConfig_PB0(void);
void APP_GpioConfig_PB6(void);

void APP_GpioConfig_PF0(void);


void APP_GpioConfig_PA1(void);

void APP_GpioConfig_PA4(void);


void user_led_invert(void);



#endif /* __USER_BSP_GPIO_H */




