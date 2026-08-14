



#include <stdio.h>



#include "user_bsp_uart2.h"

#include "user_bsp_type.h"

#include "user_light_pattern.h"

#include "user_common.h"

#include "user_bsp_gpio.h"


u16 volatile user_global_hue = 0;	// 当前色相 0~255，全灯共用，每帧+1，无头无尾循环
u8  volatile user_global_hue_step_cnt = 0;	   // 控制色相步进速度













