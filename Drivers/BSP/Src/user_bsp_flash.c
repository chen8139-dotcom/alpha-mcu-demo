


#include <stdio.h>

#include "py32f0xx_ll_exti.h"
#include "py32f0xx_ll_bus.h"

#include "user_bsp_flash.h"

#include "user_bsp_type.h"

#include "user_board_cfg.h"


#include "user_common.h"


#include "user_bsp_uart2.h"


//////////////////////////////////////////////////////////////













/////////////////////////////////////////////////



static uint32_t NVM_PageBase(uint32_t addr)
{
    return addr & ~(FLASH_PAGE_SIZE - 1U);
}

/* 主工程 8MHz */
static void NVM_TimingInitOnce(void)
{
    static uint8_t inited = 0;
    if (!inited)
    {
        LL_FLASH_TIMMING_SEQUENCE_CONFIG_8M();
        inited = 1;
    }
}

/**
 * @brief  通用读：从 Flash 地址 addr 读 len 字节到 buf
 */
void NVM_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)addr;
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        buf[i] = p[i];
    }
}

/**
 * @brief  通用写：向 Flash 地址 addr 写 len 字节（读整页->改->擦页->写整页）
 * @note   单次调用不允许跨页；跨页请拆成多次写
 * @retval 1 成功，0 参数非法（跨页）
 */
uint8_t NVM_Write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t page_base = NVM_PageBase(addr);
    uint32_t offset = addr - page_base;
    uint8_t page_buf[FLASH_PAGE_SIZE];
    const uint8_t *flash_page = (const uint8_t *)page_base;
    uint32_t i;

    if ((offset + len) > FLASH_PAGE_SIZE)
    {
        return 0;
    }

    /* 读整页 */
    for (i = 0; i < FLASH_PAGE_SIZE; i++)
    {
        page_buf[i] = flash_page[i];
    }

    /* 改 RAM 中目标区间 */
    for (i = 0; i < len; i++)
    {
        page_buf[offset + i] = buf[i];
    }

    LL_FLASH_Unlock(FLASH);
    NVM_TimingInitOnce();

    while (LL_FLASH_IsActiveFlag_BUSY(FLASH) == 1) {}
    LL_FLASH_EnablePageErase(FLASH);
    LL_FLASH_SetEraseAddress(FLASH, page_base);
    while (LL_FLASH_IsActiveFlag_BUSY(FLASH) == 1) {}
    LL_FLASH_DisablePageErase(FLASH);

    while (LL_FLASH_IsActiveFlag_BUSY(FLASH) == 1) {}
    LL_FLASH_EnablePageProgram(FLASH);
    LL_FLASH_PageProgram(FLASH, page_base, (uint32_t *)page_buf);
    while (LL_FLASH_IsActiveFlag_BUSY(FLASH) == 1) {}
    LL_FLASH_DisablePageProgram(FLASH);

    LL_FLASH_Lock(FLASH);
    return 1;
}
















