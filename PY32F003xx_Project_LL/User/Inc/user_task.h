







/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_TASK_H
#define __USER_TASK_H



#include "user_board_cfg.h"



void Task_512ms(void);


// ========== 10ms：第二优先级，人机交互和系统状态 ==========
// - 按键功能处理：模式切换/EEPROM/PWM
// - 系统启动/关闭事件处理 - 系统状态切换，需要及时响应
// - PWM更新 - 影响LED亮度，需要及时响应
// - LVD检测事件设置（从Task_8ms合并过来）
// 处理时间窗口：90ms（100ms - 10ms）
void Task_800us(void);



// ========== 50ms：第三优先级，ADC启动 ==========
// - ADC启动 - 定期采样，不需要太频繁
// 处理时间窗口：50ms（100ms - 50ms）
void Task_12_8ms(void);



// ========== 100ms：第四优先级，慢速管理和环境检测 ==========
// - ADC结果处理（白天/夜间判断）- 环境检测，不需要太频繁
// - 开机条件判断 - 慢速检查，不需要太频繁
// - 30分钟检查 - 慢速检查
// - LVD降档处理 - 虽然重要，但已经通过10ms检测设置了标志，100ms处理即可
// 处理时间窗口：100ms（200ms - 100ms）
void Task_25_6ms(void);

void Task_6_4ms(void);









#endif /* __USER_TASK_H */




