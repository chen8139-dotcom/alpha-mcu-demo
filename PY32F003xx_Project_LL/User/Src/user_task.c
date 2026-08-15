





#include "user_task.h"

#include "user_bsp_type.h"
#include <stdio.h>

#include "user_bsp_uart2.h"
#include "user_common.h"

#include "user_bsp_gpio.h"
#include "user_light_pattern.h"


#include "user_bsp_key.h"
#include "user_ble_pair.h"
#include "user_flash_manage.h"




void Task_800us(void)
{

//#if DEF_Develop_Release
//	printf("track_xx Task_12_8ms is running...\n");
//#endif


	KeyScan();

	if (G_Var_Evt_evt_key_long)
	{
		G_Var_Evt_evt_key_long = 0;
		
		#if DEF_Develop_Release
		printf("[PAIR] long press, factory re-provision\r\n");
		#endif
	}


	if (G_Var_Evt_evt_key_short)
	{
		G_Var_Evt_evt_key_short = 0;
		
		
		#if DEF_Develop_Release
		printf("track_xx KEY has been shortdown \r\n");
		#endif
		

	}



}



void Task_6_4ms(void)
{

	//printf("track_xx Task_6_4ms is running...\n");

}


void Task_12_8ms(void)
{

	//printf("track_xx Task_12_8ms is running...\n");

}


void Task_25_6ms(void)
{


//#if DEF_Develop_Release
//		 printf("track_xx Task_25_6ms is running...\n");
//#endif



}


void Task_512ms(void)
{


	printf("track_xx Task_512ms is running...\n");



}


void Task_500ms(void)
{
	user_led_invert();
}



















