

#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_adc.h"



#include "user_bsp_timer.h"
#include "user_bsp_adc.h"
#include "user_bsp_gpio.h"
#include "user_bsp_key.h"

#include "user_bsp_uart2.h"



#include "user_common.h"
#include "user_bsp_type.h"

#include <stdio.h>

#include "user_bsp_dma.h"
#include "py32f0xx_ll_flash.h"

#include "user_light_pattern.h"





//volatile uint8_t timer0_reload_value = 0;


//volatile u8 delay_1ms_fig = 0; 

volatile u8 delay_800us_fig = 0;
volatile u8 delay_3_2ms_fig = 0;
volatile u8 delay_6_4ms_fig = 0;  
volatile u8 delay_12_8ms_fig = 0; 
volatile u8 delay_25_6ms_fig = 0; 
volatile u8 delay_51_2ms_fig = 0; 
volatile u8 delay_102_4ms_fig = 0; 
volatile u8 delay_512ms_fig = 0; 



volatile u8 delay_800us_cnt = 0; 
volatile u8 delay_3_2ms_cnt = 0; 
volatile u8 delay_6_4ms_cnt = 0;    
volatile u8 delay_12_8ms_cnt = 0;    
volatile u8 delay_25_6ms_cnt = 0;     
volatile u8 delay_51_2ms_cnt = 0;
volatile u8 delay_102_4ms_cnt = 0;
volatile u8 delay_512ms_cnt = 0;






////////////变量001///////////////////


	
// 全局变量：亮度档位（3-10，初始值为10，表示100%亮度）
unsigned char   biD = 10;


// 全局变量：头亮度档位（0-2：小/中/大档，从EEPROM读取）
//unsigned char   touL=0;	
// 全局变量：头模式（0-2：双通道/单通道1/单通道2，从EEPROM读取）
unsigned char   touM = 0; //0;  // 固定为模式6（紫色呼吸灯） 
// 全局变量：工作状态计数器（记录系统运行时间）
volatile unsigned long   wos = 0;
// 全局变量：按键1按下时的时间戳（用于防抖）
//volatile unsigned long   stap = 0;
// 全局变量：按键3按下时的时间戳（用于防抖）
volatile unsigned long   stap1 = 0;
// 全局变量：ADC采样值（12位ADC的完整采样结果）
unsigned int AIn_AD0;
// 全局变量：ADC数据高字节
unsigned char R_AIN0_DATA_HB;	
// 全局变量：ADC数据低字节
unsigned char R_AIN0_DATA_LB;
// 全局变量：低电压检测计数器（连续检测到低电压的次数）
volatile unsigned char   juC = 0;	









// 位定义：是否夜间标志（bit0：1=夜间，0=白天）
unsigned char itsnit = 0;
// 位定义：一次标志（bit1：未使用）
unsigned char once = 0;
// 位定义：头按键按下标志（bit2：KE1按键按下标志）
//__sbit     toukp=MY_flag2:2;
// 位定义：启闭标志（bit3：1=开启，0=关闭）
unsigned char qbv = 0;
// 位定义：启动标志（bit4：用于延时控制）
unsigned char qden = 0;
// 位定义：PWM更新标志（bit5：0=需要更新PWM，1=已更新不需要更新）
unsigned char qsu = 0;
// 位定义：按键屏蔽光感标志（bit6：1=已过屏蔽期/恢复光感，0=还在屏蔽期内/屏蔽光感）
// 功能：0=屏蔽光感（即使白天也工作），1=恢复光感（按正常逻辑工作）
unsigned char dsdof = 0;
// 位定义：全头按键按下标志（bit7：KE3按键按下标志）
unsigned char G_Var_Key_key_fall_flag = 0;


void APP_SyncLogWosTicks(const char *label, u32 ticks)
{
#if DEF_Develop_Release && APP_BLE_DEBUG_LOG
	unsigned long long us;
	unsigned long long sec_total;
	u32 ms;
	u32 h;
	u32 mi;
	u32 s;

	us = (unsigned long long)ticks * (unsigned long long)USER_WOS_TICK_US;
	sec_total = us / 1000000ULL;
	ms = (u32)((us % 1000000ULL) / 1000ULL);
	h = (u32)(sec_total / 3600ULL);
	mi = (u32)((sec_total % 3600ULL) / 60ULL);
	s = (u32)(sec_total % 60ULL);
	printf("%s tick=0k%lu (0x%08lX) T=%02u:%02u:%02u.%03u\r\n",
	       label,
	       (unsigned long)ticks,
	       (unsigned long)ticks,
	       (unsigned int)h,
	       (unsigned int)mi,
	       (unsigned int)s,
	       (unsigned int)ms);
#else
	(void)label;
	(void)ticks;
#endif
}







/////////////变量002////////////////////////////////


unsigned char sleep_prepare_done = 0;  // 准备休眠是否已完成

////////////变量003/////////////////////////////////
unsigned char G_Var_simu_temp_flag = 0;



//////////////////////////////////////////////////////


volatile unsigned char dsd_flag = 0; //按键屏蔽光感相关标记









// ================== 新模式：整圈彩虹缓慢转动（参考淘宝24灯彩虹环） ==================
// 说明：
// - 本工程为 29 颗灯，我们预先为 29 个位置各准备一个 RGB 颜色，近似一个完整彩虹色环。
// - 每一帧不改变颜色表本身，只是改变"起始偏移 offset"，让整圈彩虹逆时针缓慢转动。
// - 优点：运行时只有加法和取模（通过减法实现），不做乘法/除法，适合 51 内核。
//
// 颜色表：29 点彩虹（HSV 色环 0~360° 均分 29 份，每份约12.4度）
// 颜色顺序：红→橙→黄→绿→青→蓝→紫→粉→红（环形队列，无头无尾）
// LED布局（逆时针）：右边0-6(7个) → 上边7-14(8个) → 左边15-22(8个) → 下边23-28(6个)
// 关键：点28（最后）→点0（开始）必须无缝过渡，紫色→红色过渡要自然
const u8 rainbow_ring_r_nonghou[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度，5个点，每点约12度)  // 再降约0.3%：114→113（目标<=65mA）
    113, 113, 113, 113, 113,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度，5个点) // 114,91,68,45,0 → 113,90,67,44,0
    113,  90,  67,  44,   0,  // 黄→黄绿→黄绿→黄绿→绿
    // 点10-14：绿色→青色 (120-180度，5个点)
      0,   0,   0,   0,   0,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度，5个点)
      0,   0,   0,   0,   0,  // 青→青蓝→青蓝→青蓝→蓝
    // 点20-24：蓝色→紫色 (240-300度，5个点) // 22,45,68,91,114 → 22,44,67,90,113
     22,  44,  67,  90, 113,  // 蓝→蓝紫→蓝紫→蓝紫→紫
    // 点25-28：紫色→粉红→红色 (300-360度，4个点，关键过渡区) // 114→113
    113, 113, 113, 113  // 紫→紫粉→粉红→粉红(接近红，确保与点0无缝)
};

const u8 rainbow_ring_g_nonghou[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度) // 0,16,44,72,102 → 0,16,44,72,101
      0,  16,  44,  72, 101,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度)
    113, 113, 113, 113, 113,  // 黄→黄绿→黄绿→黄绿→绿（114→113）
    // 点10-14：绿色→青色 (120-180度)
    113,  90,  67,  44,   0,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度)
      0,  22,  44,  67,  90,  // 青→青蓝→青蓝→青蓝→蓝
    // 点20-24：蓝色→紫色 (240-300度)
     90,  67,  44,  22,   0,  // 蓝→蓝紫→蓝紫→蓝紫→紫
    // 点25-28：紫色→粉红→红色 (300-360度，关键过渡区，确保与点0无缝)
      0,   0,  16,   8  // 紫→紫粉→粉红→粉红(接近红，点28的G≈8确保平滑过渡到点0的G=0)
};

const u8 rainbow_ring_b_nonghou[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度)
      0,   0,   0,   0,   0,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度)
      0,   0,   0,   0,   0,  // 黄→黄绿→黄绿→黄绿→绿
    // 点10-14：绿色→青色 (120-180度)
      0,  22,  44,  67,  90,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度)
    113, 113, 113, 113, 113,  // 青→青蓝→青蓝→青蓝→蓝（114→113）
    // 点20-24：蓝色→紫色 (240-300度)
    113, 113, 113, 113, 113,  // 蓝→蓝紫→蓝紫→蓝紫→紫（114→113）
    // 点25-28：紫色→粉红→红色 (300-360度，关键过渡区，确保与点0无缝)
    113,  90,  44,   8  // 紫→紫粉→粉红→粉红(接近红，点28的B≈8确保平滑过渡到点0的B=0)
};




// ================== 新模式：整圈彩虹缓慢转动（参考淘宝24灯彩虹环） ==================
// 说明：
// - 本工程为 29 颗灯，我们预先为 29 个位置各准备一个 RGB 颜色，近似一个完整彩虹色环。
// - 每一帧不改变颜色表本身，只是改变"起始偏移 offset"，让整圈彩虹逆时针缓慢转动。
// - 优点：运行时只有加法和取模（通过减法实现），不做乘法/除法，适合 51 内核。
//
// 颜色表：29 点彩虹（HSV 色环 0~360° 均分 29 份，每份约12.4度）
// 颜色顺序：红→橙→黄→绿→青→蓝→紫→粉→红（环形队列，无头无尾）
// LED布局（逆时针）：右边0-6(7个) → 上边7-14(8个) → 左边15-22(8个) → 下边23-28(6个)
// 关键：点28（最后）→点0（开始）必须无缝过渡，紫色→红色过渡要自然
const u8 rainbow_ring_r[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度，5个点，每点约12度)  // 再降约0.3%：114→113（目标<=65mA）
    255, 220, 185, 150, 113,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度，5个点) // 114,91,68,45,0 → 113,90,67,44,0
    113,  90,  67,  44,   0,  // 黄→黄绿→黄绿→黄绿→绿
    // 点10-14：绿色→青色 (120-180度，5个点)
      0,   0,   0,   0,   0,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度，5个点)
      0,   0,   0,   0,   0,  // 青→青蓝→青蓝→青蓝→蓝
    // 点20-24：蓝色→紫色 (240-300度，5个点) // 22,45,68,91,114 → 22,44,67,90,113
     22,  44,  67,  90, 113,  // 蓝→蓝紫→蓝紫→蓝紫→紫
    // 点25-28：紫色→粉红→红色 (300-360度，4个点，关键过渡区) // 114→113
    113, 184, 255, 255  // 紫→紫粉→粉红→粉红(接近红，确保与点0无缝)
};

const u8 rainbow_ring_g[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度) // 0,16,44,72,102 → 0,16,44,72,101
      0,  16,  44,  72, 101,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度)
    113, 113, 113, 113, 113,  // 黄→黄绿→黄绿→黄绿→绿（114→113）
    // 点10-14：绿色→青色 (120-180度)
    113,  95,  77,  59,   41,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度)
      23,  30,  37,  67,  90,  // 青→青蓝→青蓝→青蓝→蓝
    // 点20-24：蓝色→紫色 (240-300度)
     90,  67,  44,  22,   0,  // 蓝→蓝紫→蓝紫→蓝紫→紫
    // 点25-28：紫色→粉红→红色 (300-360度，关键过渡区，确保与点0无缝)
      4,   2,  0,   0  // 紫→紫粉→粉红→粉红(接近红，点28的G≈8确保平滑过渡到点0的G=0)
};

const u8 rainbow_ring_b[MAX_LED_COUNT] =
{
    // 点0-4：红色→橙色→黄色 (0-60度)
      0,   0,   0,   0,   0,  // 红→红橙→橙→橙黄→黄
    // 点5-9：黄色→绿色 (60-120度)
      0,   0,   0,   0,   0,  // 黄→黄绿→黄绿→黄绿→绿
    // 点10-14：绿色→青色 (120-180度)
      0,  4,  8,  12,  18,  // 绿→绿青→绿青→绿青→青
    // 点15-19：青色→蓝色 (180-240度)
    113, 113, 113, 113, 113,  // 青→青蓝→青蓝→青蓝→蓝（114→113）
    // 点20-24：蓝色→紫色 (240-300度)
    113, 113, 113, 113, 113,  // 蓝→蓝紫→蓝紫→蓝紫→紫（114→113）
    // 点25-28：紫色→粉红→红色 (300-360度，关键过渡区，确保与点0无缝)
    113,  57,  0,   0  // 紫→紫粉→粉红→粉红(接近红，点28的B≈8确保平滑过渡到点0的B=0)
};




// 模式6的速度档位索引（默认档位0）
unsigned char mode_speed_index = 0;
//> 这 8 个点大致覆盖了红、黄、白、绿、青、蓝、紫等颜色，你可以随时改成自己喜欢的 RGB。

u8 ws2812b_rgb_flag = 0;
u8 ws2812b_rgb_flag2 = 1;//0;
//static u8 first_run_flag = 0; // 全局变量声明

u8 led_mode_run_once = 0; // 全局变量声明


u8 ws2812b_reset_state_set_once = 0;
// ✅ 新增：WS2812B速度控制计数器
u8 ws2812b_cnt = 0;


// 第一次进来 / 从外部重置进来：从第 0 个子模式开始

//u8 red_hold_var = 0;//3; //20; [计划：此变量可以屏蔽掉]

int ws2812b_reset_state = -1;//0; // 0:正常, 1:发送全灭帧, 2:等待一拍后再跑模式


u8 user_led_flag = 0;
u8 user_boot_flag = 0;



//////////////////////////////////////////////////////////////////////////////////////////
/* Private variables ---------------------------------------------------------*/
uint8_t g_Usart1RxDmaBuf[USART1_RXBUFF_SIZE] = {0};
uint8_t g_Usart1FrameBuf[USART1_RXBUFF_SIZE] = {0};
uint8_t g_Usart1TxBuf[USART1_TXBUFF_SIZE] = {0};

volatile uint16_t g_Usart1FrameLen = 0;
volatile uint8_t g_Usart1FrameReady = 0;

/* Private user code ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////////////////

volatile uint8_t user_global_OB_GPIO_PIN_MODE = 0;



// 毫秒级别带形参的延时函数,实测1.06ms
void Delay_ms(unsigned int ms)
{
    while (ms--)
    {
        for (unsigned int i = 0; i < 800; i++)  //736
        {
            __NOP();__NOP();__NOP();__NOP();
        }
    }
}









void user_sys_init(void)
{

	 //uint32_t vcc_mv_dbg;
	 //uint16_t pa0_mv_dbg;
	 //uint16_t raw_vref_dbg;


	 DISI();


	 //if (user_global_OB_GPIO_PIN_MODE  == OB_GPIO_PIN_MODE) 
	 //{
	    /* Enable TIM1 clock */
	 	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);
	 //}


	   
	   
	 /* Configure system clock */
	 APP_SystemClockConfig();





	 /* Enable SYSCFG clock */
	 LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
	 
	 LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);


	 /* user LED init */
	 APP_GpioConfig_PA4();

	 
	
	 /* Initialize debug USART (used for printf) */
	 BSP_USART_Config();




	 printf("track_xx user_sys_init_1_1... \n");




	 /* Enable Dma Init for All */
	 APP_Dma_Init();
	   
	 /* Configure and enable TIM1 counter mode */
	 APP_ConfigTIM1Count();


	 //////////////////////////////

	 /* Interrupt configuration */
  	 APP_ConfigureExti();



	 ///////////////////////////////
	 
	 /*Configure USART */
	 APP_ConfigUsart(USART1);
	 
	 /* Start RX DMA in circular mode + IDLE frame detect */
	 APP_UsartReceive_DMA(USART1, (uint8_t *)g_Usart1RxDmaBuf, USART1_RXBUFF_SIZE);
	 
	 //APP_UsartTransmit(USART1, (const uint8_t*)aTxStartMessage, TXSTARTMESSAGESIZE);
	 
	 



	 /******* 10.全局变量初始化 ********/

	
	
 	/******* 11. 使能全局中断       ********/
	// 使能全局中断
	ENI();	


	



}






/**
  * @brief  Configure system clock
  * @param  None
  * @retval None
  */
void APP_SystemClockConfig(void)
{

  //#if DEF_NRST_OnOff
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_8MHz);
  //#endif



  /* Enable HSI */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Configure HSISYS as system clock source */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_Init1msTick(8000000);

  /* Update system clock global variable SystemCoreClock (can also be updated by calling SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(8000000);
}












/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void APP_ErrorHandler(void)
{
  /* infinite loop */
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     for example: printf("Wrong parameters value: file %s on line %d\r\n", file, line)  */
  /* infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */







