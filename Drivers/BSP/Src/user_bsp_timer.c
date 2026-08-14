


#include "py32f0xx_ll_tim.h"

#include "user_bsp_timer.h"
#include "user_bsp_gpio.h"


#include "user_common.h"







/**
  * @brief  Configure TIM count mode
  * @param  None
  * @retval None
  */
void APP_ConfigTIM1Count(void)
{
  /* Configure TIM1 */
  LL_TIM_InitTypeDef TIM1CountInit = {0};
  
  TIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1; /* No clock division */
  TIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;     /* Up counting */
  TIM1CountInit.Prescaler           = 8-1;                    /* Prescaler value: 8000 */
  TIM1CountInit.Autoreload          = 400-1;                    /* Auto-reload value:1000 */
  TIM1CountInit.RepetitionCounter   = 0;                         /* Repetition counter value: 0 */
  
  /* Initialize TIM1 */
  LL_TIM_Init(TIM1,&TIM1CountInit);

  /* Clear the update flag bit */
  LL_TIM_ClearFlag_UPDATE(TIM1);
  
  /* Enable preloading for automatic reloading. */
  LL_TIM_EnableARRPreload(TIM1);
  
  /* Enable UPDATE interrupt */
  LL_TIM_EnableIT_UPDATE(TIM1);
  
  NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn,0);

  
  /* Enable UPDATE interrupt request */
  NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);


    /* Enable TIM1 counter */
  LL_TIM_EnableCounter(TIM1);
}



/**
  * @brief  Period elapsed callback in non blocking mode 
  * @param  None
  * @retval None
  */
void APP_UpdateCallback(void)
{


    /*USER BEGIN */

	static uint8_t user_led_reverse = 0;





	// 工作状态计数器加1
	wos++;




    // ========== 设置时间标志位（嵌套if结构：1ms → 10ms → 50ms → 100ms → 200ms）==========
    //delay_1ms_fig = 1;				 //1ms标志位 


	//LEDB = !LEDB;


	if (user_led_reverse == 0) {
		user_led_reverse = 1;
		//LEDB_H;
	} else {
		user_led_reverse = 0;
		//LEDB_L;
	}

	

	if (++delay_800us_cnt >= 2) 	 //800 us = 400us * 2
	{
			delay_800us_cnt = 0x00;
			delay_800us_fig = 1; 		//5 ms标志位

			

		   if (++delay_3_2ms_cnt >= 4) 	//3.2 ms = 800 us * 4
		   {
			   delay_3_2ms_cnt = 0x00;	   
			   delay_3_2ms_fig = 1; 		//3.2ms标志位
	

	 			
			   
			   if (++delay_6_4ms_cnt >= 2)	 //6.4 ms = 3.2 ms * 2
			   {
				   delay_6_4ms_cnt = 0x00;	  
				   delay_6_4ms_fig = 1; 	 //6.4 ms 标志位
	
				   //LEDB = !LEDB;


				  

				   
				   
				   if (++delay_12_8ms_cnt >= 2)  // 12.8 ms = 6.4 ms * 2
				   {
					   delay_12_8ms_cnt = 0x00;    
					   delay_12_8ms_fig = 1;	 //12.8 ms 标志位
	
					   
		  
					   if (++delay_25_6ms_cnt >= 2)  // 25.6 ms = 12.8 ms * 2
					   {
						   delay_25_6ms_cnt = 0x00;
						   delay_25_6ms_fig = 1;  // 25.6 ms 标志位


						   if (++delay_51_2ms_cnt >= 2)  // 51.2 ms = 25.6 ms * 2
						   {
							   delay_51_2ms_cnt = 0x00;
							   delay_51_2ms_fig = 1;  // 51.2 ms 标志位



							   if (++delay_102_4ms_cnt >= 2)  // 102 ms = 51.2 ms * 2
							   {
								   delay_102_4ms_cnt = 0x00;
								   delay_102_4ms_fig = 1;  // 102 ms 标志位


								   if (++delay_512ms_cnt >= 5)  // 512 ms = 102 ms * 5
								   {
									   delay_512ms_cnt = 0x00;
									   delay_512ms_fig = 1;  // 512 ms 标志位


								   }


							   }


						   }
						   	




						   
						   
					   }
		   
						   
				   }   
				   
				   
			   } 
			   
			   
		   }

	}



	/*USER END */
        

    

  

    
}





































