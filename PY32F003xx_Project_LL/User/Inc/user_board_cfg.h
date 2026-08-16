




/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_BOARD_CFG_H
#define __USER_BOARD_CFG_H


/* Includes ------------------------------------------------------------------*/
//#include "py32f0xx.h"








#define DEF_SOP8_SOP16	  0   //0: SOP8; 1:SOP16

#define DEF_SOP20_PACKAGE  1   //0: SOP8(PB5); 1:SOP20(PA12)，仅 DEF_SOP8_SOP16=0 时有效

#define DEF_SOP16_GuoDu	  1   //0:不过渡; 1:过渡

#define DEF_NRST_OnOff	  1   //0:关闭; 1:开启





#define NVM_DEMO_TEST            1 // 0


#define USER_LED_TEST            0 // LED is managed by the Beacon Mesh state machine


#endif /* __USER_BOARD_CFG_H */






