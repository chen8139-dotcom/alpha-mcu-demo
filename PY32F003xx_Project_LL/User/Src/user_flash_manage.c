/**
  ******************************************************************************
  * @file    user_flash_manage.c
 * @brief   Flash 用户 NVM：配网标志 + NetworkID 读写 + 可选演示
  ******************************************************************************
  */

#include "user_flash_manage.h"

#include <stdio.h>
#include <string.h>

#include "user_bsp_flash.h"
#include "user_common.h"





#if NVM_DEMO_TEST

#define NVM_BYTE3_ADDR            (NVM_PAGE_BASE_ADDR + 0x61U)
#define NVM_BYTE3_VALUE           0xEDU





static void NVM_DumpPage(void)
{
	const uint8_t *p = (const uint8_t *)NVM_PAGE_BASE_ADDR;

	printf("[FLASH PAGE DUMP] 0x%08X ~ 0x%08X\r\n",
	       (unsigned int)NVM_PAGE_BASE_ADDR,
	       (unsigned int)(NVM_PAGE_BASE_ADDR + FLASH_PAGE_SIZE - 1U));

	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16U)
	{
		printf("0x%08X: ", (unsigned int)(NVM_PAGE_BASE_ADDR + i));
		for (uint32_t j = 0; j < 16U; j++)
		{
			printf("%02X ", p[i + j]);
		}
		printf("\r\n");
	}
	printf("\r\n");
}






void NVM_DemoRealScenario(void)
{
	uint8_t ed_rb;
	const uint8_t ED_val = NVM_BYTE3_VALUE;


	printf("\r\n NVM_DemoRealScenario_ NVM_DumpPage ...before: \r\n\r\n");
	NVM_DumpPage(); 

	NVM_Read(NVM_BYTE3_ADDR, &ed_rb, 1);

	printf("NVM_DemoRealScenario_ 0x%08X: 0x%02X\r\n\r\n\r\n\r\n", (unsigned int)NVM_BYTE3_ADDR, ed_rb);




	printf("\r\n NVM_Write byte 0xED \r\n\r\n\r\n");
	if (!NVM_Write(NVM_BYTE3_ADDR, &ED_val, 1))
	{
		return;
	}




	printf("\r\n NVM_DemoRealScenario_ NVM_DumpPage ...after: \r\n\r\n");
	NVM_DumpPage();


}






#endif /* NVM_DEMO_TEST */
