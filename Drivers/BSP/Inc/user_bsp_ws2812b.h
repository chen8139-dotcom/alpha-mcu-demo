



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BSP_WS2812B_H
#define __USER_BSP_WS2812B_H


//#include "py32f0xx.h"   // 包含GPIO相关的寄存器定义，例如 GPIOB->BSRR 使用
#include "py32f0xx_ll_gpio.h"




#include "user_board_cfg.h"





//GPIOB->BSRR = (1U << 0); 

//GPIOB->BSRR = (1U << (0 + 16));



void ws2812b_Init(void);




#endif /* __USER_BSP_WS2812B_H */




