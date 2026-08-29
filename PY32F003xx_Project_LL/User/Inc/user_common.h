




/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_COMMON_H
#define __USER_COMMON_H


//#include "py32f003xx_ll_Start_Kit.h"

#include "user_bsp_type.h"

#include "user_bsp_uart1.h"


#include "user_board_cfg.h"



///////////////// 宏定义 001 ///////////////////////////////


#define  DEF_Develop_Release	1 // 1:研发阶段 0:发行版本;
#define  DEF_zuoxiu		        0 // 1:作秀 0:不作秀;

#ifndef APP_BLE_DEBUG_LOG
#define APP_BLE_DEBUG_LOG        1U  /* 可被 main.c 或 Keil 预定义覆盖；影响 APP_SyncLogWosTicks 等调试输出 */
#endif
#ifndef APP_TASK_512MS_WOS_LOG
#define APP_TASK_512MS_WOS_LOG   0U  /* 1: Task_512ms 每 512ms 打印 wos tick；0: 关闭 */
#endif
#ifndef APP_MESH_TRACE_LOG
#define APP_MESH_TRACE_LOG        1U  /* 第二迭代 Mesh 队列/Relay/去重/时序追踪日志 */
#endif





///////////////// 宏定义 ///////////////////////////////


// 宏定义：按键屏蔽光感时间阈值（12000个定时器周期，约4.8秒，接近5秒）
// 产品需求：按键按下后屏蔽光感5秒，然后重新开启光感功能
#define hsv 12000UL	 //[400us定时器周期]	



// 宏定义：定时器0重装载值（200，即每200个计数产生一次中断）
// 定时器0配置：指令时钟8MHz，16分频后=500kHz，周期=2μs
// 中断周期：200 × 2μs = 400μs = 0.4ms
#define QTD    200             
// 宏定义：定时器0初始值（256 - 200 = 56）
#define QT0R (256 - QTD)  

/** wos 每递增 1 的间隔（µs），须与 Timer0 400µs 时基一致 */
#ifndef USER_WOS_TICK_US
#define USER_WOS_TICK_US 400U
#endif







// 当前Timer0重装载值（默认正常模式）


#define QTD_NORMAL    30   // 正常模式计数增量
#define QT0R_NORMAL   (256 - QTD_NORMAL)  // = 226
#define T0MD_NORMAL   //(C_PS0_TMR0 | C_PS0_Div128)  // 128分频   //[工作态T=1ms]


#define QTD    200             
// 宏定义：定时器0初始值（256 - 200 = 56）
#define QT0R (256 - QTD)  









//volatile uint8_t timer0_reload_value = 0;





/* Private define ------------------------------------------------------------*/
/* #define OB_GPIO_PIN_MODE LL_FLASH_NRST_MODE_RESET */
#define OB_GPIO_PIN_MODE LL_FLASH_NRST_MODE_GPIO




/////////////// extern 全局变量 begin ////////////////////////////////

/////////////////////////////




///////////////////////////


//extern volatile u8 delay_1ms_fig = 0; 

extern volatile unsigned char delay_800us_fig;
extern volatile unsigned char delay_3_2ms_fig;
extern volatile unsigned char delay_6_4ms_fig;  
extern volatile unsigned char delay_12_8ms_fig; 
extern volatile unsigned char delay_25_6ms_fig; 
extern volatile unsigned char delay_51_2ms_fig; 
extern volatile unsigned char delay_102_4ms_fig; 
extern volatile unsigned char delay_512ms_fig; 
extern volatile unsigned char delay_500ms_fig;



extern volatile unsigned char delay_800us_cnt; 
extern volatile unsigned char delay_3_2ms_cnt; 
extern volatile unsigned char delay_6_4ms_cnt;    
extern volatile unsigned char delay_12_8ms_cnt;    
extern volatile unsigned char delay_25_6ms_cnt;     
extern volatile unsigned char delay_51_2ms_cnt;
extern volatile unsigned char delay_102_4ms_cnt;
extern volatile unsigned char delay_512ms_cnt;
extern volatile unsigned short delay_500ms_cnt;

//////////////////////////////////////


////////////变量001///////////////////


	
// 全局变量：亮度档位（3-10，初始值为10，表示100%亮度）
extern unsigned char   biD;


// 全局变量：头亮度档位（0-2：小/中/大档，从EEPROM读取）
//unsigned char   touL;	
// 全局变量：头模式（0-2：双通道/单通道1/单通道2，从EEPROM读取）
extern unsigned char   touM; //0;  // 固定为模式6（紫色呼吸灯） 
// 全局变量：工作状态计数器（记录系统运行时间）
extern volatile unsigned long   wos;

/** 把 wos tick 打成「0k十进制 + 十六进制 + 伪时间 T=HH:MM:SS.mmm」（与 USER_WOS_TICK_US 一致，非 RTC）；供同步与任务调试共用 */
void APP_SyncLogWosTicks(const char *label, u32 ticks);

// 全局变量：按键1按下时的时间戳（用于防抖）
//volatile unsigned long   stap;
// 全局变量：按键3按下时的时间戳（用于防抖）
extern volatile unsigned long   stap1;















// 位定义：全头按键按下标志（bit7：KE3按键按下标志）
extern unsigned char G_Var_Key_key_fall_flag;






/////////////变量002////////////////////////////////



////////////变量003/////////////////////////////////
extern unsigned char G_Var_simu_temp_flag;



//////////////////////////////////////////////////////














extern u8 user_led_flag;
extern u8 user_boot_flag;




////////////////////////////////////////////////

extern u8  g_Usart1RxDmaBuf[USART1_RXBUFF_SIZE];
extern u8  g_Usart1FrameMailbox[APP_USART1_FRAME_MAILBOX_CAPACITY][USART1_RXBUFF_SIZE];
extern u8  g_Usart1TxBuf[USART1_TXBUFF_SIZE];
extern u8  aTxStartMessage[];
extern u8  aTxEndMessage[];
extern volatile unsigned short g_Usart1FrameMailboxLen[APP_USART1_FRAME_MAILBOX_CAPACITY];
extern volatile u8 g_Usart1FrameMailboxHead;
extern volatile u8 g_Usart1FrameMailboxTail;
extern volatile u8 g_Usart1FrameMailboxCount;





///////////////////////////////////////////////

//volatile uint8_t timer0_reload_value;

extern volatile uint8_t user_global_OB_GPIO_PIN_MODE;




/////////////// extern 全局变量 end ////////////////////////////////


void Delay_ms(unsigned int ms);


void user_sys_init(void);




void APP_SystemClockConfig(void);

void APP_ErrorHandler(void);






















#endif /* __USER_COMMON_H */






