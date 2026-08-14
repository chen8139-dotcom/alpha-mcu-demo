


#include <stdio.h>
#include "py32f0xx_ll_bus.h"

#include "user_bsp_gpio.h"

#include "user_bsp_type.h"







extern u8 user_led_flag;


extern u8 user_boot_flag;




/* ***************************** */
/* Gpio_Clock_Init */
/* ***************************** */
void APP_Gpio_Clock_Init(void)
{

	/* Enable clock GPIOA */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);


	/* Enable clock GPIOB */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);


	//#if DEF_SOP8_SOP16


		//#if DEF_SOP16_GuoDu
		/* Enable clock GPIOB */
		
		//#if DEF_NRST_OnOff
		LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
		//#endif
		
		
		//#endif


	//#endif

}






/* ***************************** */
/* WS2812B RGB_DATA IO Init */
/* ***************************** */
void APP_GpioConfig_PB0(void)
{


	#if 0
	/* Enable clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Default (after reset) is no pull-up or pull-down */
	/* LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_5, LL_GPIO_PULL_NO); */

	//WS_IO_H;
	#endif

	/**
	  * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	 */

	// PF2
  	//LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG);



	/* 初始化 ws data IO为 高电压，然后切换为输出模式，目的是不让WS随意复位 */

	


	WS_IO_H;
	/* Default (after reset) is no pull-up or pull-down */
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_0, LL_GPIO_PULL_DOWN);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);


	



	user_boot_flag = 1;
	//#endif

  
}




/* ***************************** */
/* WS2812B RGB_DATA IO Init */
/* ***************************** */
void APP_GpioConfig_PB6(void) //【淘宝开发板 RGB_DATA】
{


	#if 0
	/* Enable clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Default (after reset) is no pull-up or pull-down */
	/* LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_5, LL_GPIO_PULL_NO); */

	//WS_IO_H;
	#endif

	/**
	  * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	 */

	// PF2
  	//LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG);



	/* 初始化 ws data IO为 高电压，然后切换为输出模式，目的是不让WS随意复位 */

	


	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_6);


	



	user_boot_flag = 1;
	//#endif

  
}




   
/* ***************************** */
/* WS2812B RGB_DATA IO Init */
/* ***************************** */
void APP_GpioConfig_PF0(void)
{


	#if 0
	/* Enable clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Default (after reset) is no pull-up or pull-down */
	/* LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_5, LL_GPIO_PULL_NO); */

	//WS_IO_H;
	#endif



	/* 初始化 ws data IO为 高电压，然后切换为输出模式，目的是不让WS随意复位 */

	#if 0
	// PF2
  	LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG);


	WS_IO_H;
	/* Default (after reset) is no pull-up or pull-down */
	LL_GPIO_SetPinPull(GPIOF, LL_GPIO_PIN_0, LL_GPIO_PULL_DOWN);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOF, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOF, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);



	/**
	  * 根据数据手册第20页, 同管脚的其它PIN应当设为 ANALOG.
	 */
	#endif

	/* Enable clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	/* Configure PB5 pin as output */
	LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
	/* Default (after reset) is push-pull output */
	LL_GPIO_SetPinOutputType(GPIOF, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL); 
	/* Configure PB5 output speed as very high */
	LL_GPIO_SetPinSpeed(GPIOF, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	/* Default (after reset) is no pull-up or pull-down */
	/* LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_5, LL_GPIO_PULL_NO); */

	//WS_IO_H;

		// PF2
  	LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG);



	user_boot_flag = 1;

  
}














/* ***************************** */
/* WS2812B RGB_POWER_CE IO Init  */
/* ***************************** */
void APP_GpioConfig_PA1(void)
{

	#if 0
	/* Enable clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

	/* Configure PA1 as output */
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);

	WS_SWITCH_L;
	#endif


	LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_1, LL_GPIO_PULL_UP);
	LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_1);

  
}


/* ***************************** */
/* WS2812B LEDB IO Init  */
/* ***************************** */
void APP_GpioConfig_PA4(void)
{


  /* Configure PA1 as output */
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_4, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_4, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_4, LL_GPIO_SPEED_FREQ_LOW);


  LEDB_H;
}




void user_led_invert(void)
{

	if (user_led_flag == 0)
	{
		user_led_flag = 1;
		LEDB_L;
	} 
	else 
	{
		user_led_flag = 0; 
		LEDB_H;
	}


}










