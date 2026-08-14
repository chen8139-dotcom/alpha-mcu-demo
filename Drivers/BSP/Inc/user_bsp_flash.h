



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BSP_FLASH_H
#define __USER_BSP_FLASH_H


//#include "py32f0xx.h"   // 包含GPIO相关的寄存器定义，例如 GPIOB->BSRR 使用
#include "py32f0xx_ll_flash.h"
#include "user_board_cfg.h"


#ifndef DEF_SOP8_SOP16
#error "DEF_SOP8_SOP16 is not defined. Please include user_board_cfg.h first."
#endif







void NVM_Read(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t NVM_Write(uint32_t addr, const uint8_t *buf, uint32_t len);






#endif /* __USER_BSP_FLASH_H */




