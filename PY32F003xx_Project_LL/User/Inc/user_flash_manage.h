/**
  ******************************************************************************
  * @file    user_flash_manage.h
 * @brief   Flash 用户 NVM 区：配网标志 + NetworkID
  ******************************************************************************
  */

#ifndef USER_FLASH_MANAGE_H
#define USER_FLASH_MANAGE_H

#include <stdint.h>


#include "user_board_cfg.h"


/* PY32F003x8：64KB Flash，用户 NVM 放在最后一页，与程序区隔离 */
#define NVM_PAGE_BASE_ADDR        0x0800FF80U
#define FLASH_USER_START_ADDR     NVM_PAGE_BASE_ADDR

/* 配网记录（页内偏移 +0x00） */
#define NVM_RECORD_ADDR           (NVM_PAGE_BASE_ADDR + 0x00U)
#define NVM_MAGIC_VALUE           0xA5U
#define NVM_PROVISIONED_VALUE     0x01U








#if NVM_DEMO_TEST

void NVM_DemoRealScenario(void);

#endif

#endif /* USER_FLASH_MANAGE_H */
