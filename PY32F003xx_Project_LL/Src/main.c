/**

  ******************************************************************************

  * @file    main.c

  * @author  MCU Application Team

  * @brief   Main program body

  ******************************************************************************

  */



/* Includes ------------------------------------------------------------------*/

#include "main.h"

#include <stdio.h>

#include "user_common.h"

#include "user_bsp_adc.h"

#include "user_bsp_timer.h"

#include "user_task.h"

#include "user_bsp_gpio.h"

#include "user_bsp_uart2.h"

#include "user_light_pattern.h"

#include "user_bsp_ws2812b.h"

#include "user_ble_pair.h"

#include "user_ble_uart.h"

//#include "py32f0xx_ll_flash.h"

#include "user_bsp_flash.h"

#include "user_flash_manage.h"



/**

  * @brief  Main program.

  * @retval int

  */

int main(void)

{

	APP_Gpio_Clock_Init();
	Delay_ms(10);
	ws2812b_Init();

	 



	user_sys_init();

	




	Delay_ms(5);



	printf("hello PY32F003\r\n");




#if NVM_DEMO_TEST
	NVM_DemoRealScenario();
#endif








#if USER_LED_TEST

	LEDB_L;
	Delay_ms(1);

	LEDB_H;
	Delay_ms(1);

	LEDB_L;
	Delay_ms(1);

	LEDB_H;
	Delay_ms(1);


#endif
	



	while (1)
	{
		APP_HandleBleUartFrame();
	

		if (delay_800us_fig)   { delay_800us_fig = 0;  Task_800us(); }
		if (delay_6_4ms_fig)   { delay_6_4ms_fig = 0;  Task_6_4ms(); }
		if (delay_12_8ms_fig)  { delay_12_8ms_fig = 0; Task_12_8ms(); }
		if (delay_25_6ms_fig)  { delay_25_6ms_fig = 0; Task_25_6ms(); }
		if (delay_512ms_fig)   { delay_512ms_fig = 0;  Task_512ms(); }
	}
}



/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
