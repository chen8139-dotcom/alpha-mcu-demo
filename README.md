# PY32F0xx Firmware V1.5.0

基于普冉 `PY32F003F18P6` 的 MCU 基础驱动与应用框架工程，面向 `PY32F003F1xP_START_V1.1` 开发板硬件开发。项目使用 CMSIS、PY32F0xx LL 驱动和自定义 BSP，提供 GPIO、定时器、按键、UART、DMA、Flash NVM 等基础能力，并在此基础上搭建了一个面向产品应用的时间片轮询框架。

> 本 README 以当前源码和 Keil 工程配置为准。附件《260812_2_MCU基础驱动例程框架介绍.docx》用于补充示例现象、目录分层和开发建议；附件中的建议不等同于当前工程已经实现的全部功能。

## 1. 项目定位

- MCU：PY32F003 系列，当前 Keil 目标为 `PY32F003x8`
- 开发板：`PY32F003F1xP_START_V1.1`
- 板载芯片：`PY32F003F18P6`，TSSOP20，64 KB Flash，8 KB SRAM
- CPU：Arm Cortex-M0+
- 驱动模型：CMSIS + PY32F0xx LL（Low Layer）驱动
- 主要开发环境：Keil MDK-ARM 工程
- 应用入口：`PY32F003xx_Project_LL/Src/main.c`
- 应用框架：中断产生周期标志，主循环按时间片调用任务
- 当前用途：基础外设验证、串口协议调试、按键交互、Flash 掉电存储验证和产品功能框架开发

> 注意：开发板实物芯片为 `PY32F003F18P6`，而 Keil 工程当前设备选择显示为 `PY32F003x8`。两者均属于 PY32F003 系列，当前固件已有构建记录；正式量产或调整存储布局前，应在所用 DFP 中再次确认精确料号、封装和 Flash 参数是否匹配。本次只补充文档，不自动修改工程设备配置。

## 2. 快速开始

### 2.1 打开工程

使用 Keil uVision 打开：

```text
PY32F003xx_Project_LL/MDK-ARM/Project.uvprojx
```

工程配置的关键参数如下：

| 项目 | 当前配置 |
| --- | --- |
| Target | `PY32F003x8` |
| 编译宏 | `PY32F003x8,USE_FULL_LL_DRIVER` |
| 工程使用的 DFP | `Puya.PY32F0xx_DFP.1.2.8` |
| 工程输出名 | `Project` |
| 下载/调试 | 工程配置使用 J-Link 相关设置 |

仓库中还提供了设备包和工具包：

- `Packs/MDK/Puya.PY32F0xx_DFP.1.2.6.pack`
- `Packs/IAR/PY32F0xx_IAR_DFP_V1.0.0.zip`
- `Packs/GCC/openocd-0.12.0.zip`

> 当前仓库中可以直接打开并验证的是 MDK 工程。GCC、IAR 目录主要提供设备包或辅助工具，不代表已经提供了同等配置的 GCC/IAR 构建工程。

### 2.2 编译与下载

1. 安装与工程配置匹配的 Puya PY32F0xx DFP。
2. 打开 `Project.uvprojx`，确认目标芯片为 `PY32F003x8`。
3. 执行 **Build** 或 **Rebuild**。
4. 通过 J-Link 下载并运行。
5. 使用串口工具观察 UART2 调试输出；UART1 可连接蓝牙模组或串口工具进行协议测试。

日志串口的完整配置、日志来源、开关和排查方法见 [`doc/LOGGING.md`](doc/LOGGING.md)。

已有构建日志 `PY32F003xx_Project_LL/MDK-ARM/build_log.txt` 显示一次构建结果为：

```text
0 Error(s), 0 Warning(s)
```

该日志记录的编译器为 ARMCC `V5.06 update 6 (build 750)`。实际使用时应以本机 Keil、编译器和 DFP 版本为准。

### 2.3 构建产物

构建产物位于 `PY32F003xx_Project_LL/MDK-ARM` 下的以下目录：

| 产物 | 路径 | 用途 |
| --- | --- | --- |
| ELF/AXF | `Objects/Project.axf` | 调试和符号信息 |
| HEX | `Objects/Project.hex` | 常用下载/烧录格式 |
| BIN | `bin/Project.bin` | 二进制固件 |
| 校验文件 | `bin/Project.chk` | 构建后脚本生成的 BIN 校验结果 |
| MAP | `Listings/Project.map` | 链接映射和内存占用分析 |

工程的构建后命令会调用 `MDK-ARM/after_build_bin_sum16.bat`，对 BIN 生成 16 位校验结果。

## 3. 工程目录

```text
PY32F0xx_Firmware_V1.5.0/
├── Documentation/                    # PY32F002A/PY32F003/PY32F030 用户手册
├── devkitdoc/                         # 本开发板原理图
├── Drivers/
│   ├── CMSIS/                        # CMSIS 核心、设备头文件和 DSP 组件
│   ├── PY32F0xx_HAL_Driver/          # PY32F0xx LL 驱动源码与头文件
│   └── BSP/
│       ├── Inc/                      # GPIO、UART、DMA、定时器、按键、Flash 等 BSP 接口
│       └── Src/                      # BSP 实现
├── Packs/                            # MDK/IAR/GCC 相关设备包或工具
├── PY32F003doc/                       # PY32F003 数据手册、参考手册和 LL 样例
├── PY32F003xx_Project_LL/
│   ├── Inc/                          # 工程入口头文件和工程配置
│   ├── Src/                          # main、系统文件和中断文件
│   ├── User/
│   │   ├── Inc/                      # 产品配置、任务和应用模块接口
│   │   └── Src/                      # 应用逻辑
│   ├── MDK-ARM/                      # Keil 工程、调试配置和构建输出
│   └── virtualforsi/                 # 仿真/兼容环境使用的基础头文件
└── README.md
```

代码分层关系如下：

```text
main.c / py32f0xx_it.c
        │
        ├── User：应用逻辑、协议处理、任务和产品配置
        │       └── user_common / user_task / user_ble_uart / user_flash_manage
        │
        ├── BSP：板级外设封装
        │       └── GPIO / UART1 / UART2 / DMA / TIM1 / Key / Flash / WS2812B
        │
        ├── PY32F0xx LL Driver：芯片外设寄存器级驱动
        └── CMSIS：Cortex-M0+、设备定义和通用组件
```

## 4. 开发板硬件与默认配置

本项目针对 `PY32F003F1xP_START_V1.1` 开发板开发。硬件连接以 `devkitdoc/PY32F003F1xP_START_V1.1_SCH(1).pdf` 原理图为准，固件行为以 `user_board_cfg.h`、`user_bsp_gpio.h`、`user_bsp_uart1.c` 和 `user_bsp_uart2.h` 的当前配置为准。

### 4.1 开发板和芯片信息

| 项目 | 信息 |
| --- | --- |
| 开发板 | `PY32F003F1xP_START_V1.1` |
| 主控芯片 | `PY32F003F18P6` |
| 封装 | TSSOP20 |
| Flash | 64 KB |
| SRAM | 8 KB |
| 供电 | USB Type-C 输入，板载 LM1117-3.3 稳压 |
| 调试接口 | SWD：SWDIO、SWCLK、NRST、VDD、GND |
| 外部晶振 | 24 MHz，连接 OSCIN/OSCOUT |

### 4.2 原理图中的板载连接

| 板载功能 | 芯片引脚 | 原理图说明 |
| --- | --- | --- |
| 用户按键 KEY1 | PA12 | 低电平按下，配有上拉电阻，连接到扩展排针 P1 |
| 用户 LED LED2 | PB5 | 绿色 LED，经 680 Ω 电阻接 MCU_VDD，通常为低电平点亮 |
| SWD 调试 | PA13 / PA14 | PA13 为 SWDIO，PA14 为 SWCLK |
| 复位 | PF2-NRST | 板载复位按键和 NRST 引出 |
| 启动配置 | PF4-BOOT0 | 通过 Boot0 排针选择启动模式 |
| 外部晶振 | PF0 / PF1 | OSCIN/OSCOUT 连接 24 MHz 晶振 |
| 扩展排针 P1 | PA0、PA1、PA2、PA3、PA4、PA5、PA6、PA7、PA12 等 | 便于连接 UART、传感器和其他外部电路 |
| 扩展排针 P2 | PB5、PB6、PB7、SWD、BOOT0、NRST 等 | PB5/PB6/PB7 可用于外部功能扩展 |

### 4.3 开发板连接与当前固件配置对照

| 功能 | 开发板原理图 | 当前固件 | 适配状态 |
| --- | --- | --- | --- |
| 板载指示 LED | PB5（LED2） | PB5（`LEDB_H/LEDB_L`），低电平点亮 | 匹配 |
| UART1 TX/RX | PA2 / PA3，均由 P1 引出 | PA2 / PA3，115200 8N1 | 匹配 |
| UART2 调试 TX/RX | PB6 / PB7，均由 P2 引出 | PB6 / PB7，115200 8N1 | 匹配，适合 `printf` 输出和调试接收 |
| 按键 | PA12 | PA12，低电平有效 | 匹配 |
| WS2812B 数据 | 原理图未包含板载 WS2812B，PB6 仅在 P2 引出 | PB6 | 外接功能预留 |
| WS2812B 供电控制 | 原理图未包含对应板载供电电路，PA1 在 P1 引出 | PA1 | 外接功能预留 |
| 用户 Flash 页 | PY32F003F18P6 为 64 KB Flash | `0x0800FF80` 起始 | 地址位于 64 KB Flash 末页区域，仍需保留链接空间 |

当前 UART1 使用 PA2/PA3，UART2 调试使用 PB6/PB7，两个串口均连接到原理图引出的引脚。

### 4.4 固件使用的用户 Flash 区域

| 功能 | 地址/配置 | 当前说明 |
| --- | --- | --- |
| 用户 Flash 页 | `0x0800FF80` | 64 KB Flash 的最后一页区域 |
| Flash 演示字节 | `0x0800FFE1` | `0x0800FF80 + 0x61`，默认写入演示值 `0xED` |

### 4.5 固件封装和封装选择

默认配置位于 `PY32F003xx_Project_LL/User/Inc/user_board_cfg.h`：

```c
#define DEF_SOP8_SOP16  0
#define DEF_SOP20_PACKAGE  1
#define NVM_DEMO_TEST  1
#define USER_LED_TEST  1
```

修改封装相关宏后，需要同步确认按键、WS2812B 数据线以及相应 GPIO 初始化逻辑，避免只修改宏而未检查实际硬件连接。

## 5. 当前已实现功能

### 5.1 系统启动与调试输出

`main()` 的启动流程为：

1. 初始化 GPIO 时钟。
2. 初始化 LED 和相关启动延时。
3. 调用 `user_sys_init()` 完成系统时钟、UART2、DMA、TIM1、按键 EXTI 和 UART1 的初始化。
4. 输出启动日志：

```text
track_xx user_sys_init_1_1...
hello PY32F003
```

### 5.2 UART1 DMA + IDLE 接收

UART1 使用 DMA 循环接收缓冲区，USART1 IDLE 中断用于判断一帧数据结束。接收完成后，代码会：

- 将 DMA 缓冲区复制到帧缓冲区。
- 通过 UART1 原样回传当前帧。
- 通过 UART2 打印帧长度和十六进制内容。
- 在主循环中调用 `APP_HandleBleUartFrame()` 做帧头、帧尾和业务分发。

当前应用层已实现 APP 开关灯测试帧：

```text
55 AA 00 04 02 01 01 XX CHK FE
```

其中：

- `XX = 00`：开灯。
- `XX = 01`：关灯。
- `CHK`：从帧头开始累加前 8 个字节，取低 8 位。
- `FE`：帧尾。

示例：

```text
# 开灯
55 AA 00 04 02 01 01 00 07 FE

# 关灯
55 AA 00 04 02 01 01 01 08 FE
```

### 5.3 UART2 调试输出

UART2 默认波特率为 115200，数据格式为 8N1。主要用于输出 `printf` 日志、UART1 收包内容、Flash Dump 和任务运行信息。

### 5.4 按键扫描

按键在 `Task_800us()` 中执行扫描：

- 约 10 ms 按下消抖。
- 按住约 3 s 产生长按事件。
- 未达到长按阈值并释放时产生短按事件。
- 当前任务函数会输出短按和长按调试日志。

### 5.5 TIM1 时间片轮询

TIM1 更新中断递增系统节拍并设置周期标志；主循环不阻塞等待，而是按标志执行任务：

```text
TIM1 update interrupt
        │
        ├── delay_800us_fig   ──> Task_800us()
        ├── delay_6_4ms_fig   ──> Task_6_4ms()
        ├── delay_12_8ms_fig  ──> Task_12_8ms()
        ├── delay_25_6ms_fig  ──> Task_25_6ms()
        └── delay_512ms_fig   ──> Task_512ms()
```

当前 `main.c` 已分发上述 5 个任务。驱动层还生成了 3.2 ms、51.2 ms 和 102.4 ms 相关标志，但它们当前没有在主循环中单独分发；如需使用，应在确认任务执行时间和调度关系后补充调用。

这种框架适合将不同响应速度的功能拆分到不同任务中：按键和快速状态放入较短周期，环境检测和低频维护放入较长周期，避免在主循环中使用长时间阻塞延时。

### 5.6 Flash NVM 演示

当 `NVM_DEMO_TEST=1` 时，启动阶段调用 `NVM_DemoRealScenario()`：

1. Dump 用户 Flash 页。
2. 读取 `0x0800FFE1` 的原始值。
3. 写入 `0xED`。
4. 再次 Dump 用户 Flash 页，验证写入结果。

典型输出会包含：

```text
[FLASH PAGE DUMP] 0x0800FF80 ~ 0x0800FFFF
NVM_DemoRealScenario_ 0x0800FFE1: 0xFF
NVM_Write byte 0xED
```

## 6. 配置宏

主要配置文件为 `PY32F003xx_Project_LL/User/Inc/user_board_cfg.h` 和 `user_common.h`。

| 宏 | 作用 |
| --- | --- |
| `DEF_SOP8_SOP16` | 选择 SOP8/SOP16 相关 GPIO 路径；`0` 表示当前使用非 SOP16 路径 |
| `DEF_SOP20_PACKAGE` | 在非 SOP16 路径下选择 SOP20 按键 PA12；`0` 时使用 SOP8 PB5 |
| `NVM_DEMO_TEST` | 是否在启动阶段执行 Flash 读写演示 |
| `USER_LED_TEST` | 是否在启动阶段执行 LED 翻转测试 |
| `DEF_Develop_Release` | 研发/发行阶段日志条件宏；当前为研发阶段 |
| `APP_BLE_DEBUG_LOG` | UART1/BLE 业务日志开关 |
| `APP_VERBOSE_UART1_RX` | UART1 原始接收帧详细日志开关 |
| `APP_TASK_512MS_WOS_LOG` | 是否输出 512 ms 任务的节拍日志 |

正式版本建议至少关闭 `NVM_DEMO_TEST` 和不需要的详细日志，避免反复擦写用户 Flash 或产生过多串口输出。

## 7. 调试与验证步骤

### 7.1 UART2 上电日志

串口工具配置为：

```text
波特率：115200
数据位：8
停止位：1
校验位：None
```

连接 UART2 TX（PB6）后复位设备，检查是否能够看到 `hello PY32F003`、Flash Dump 和周期任务日志；如需验证 UART2 接收，将转换器 TX 连接到 PB7。

### 7.2 UART1 收发验证

将 UART1 的 PA2/PA3 连接到串口工具或蓝牙模组，配置同样的 115200 8N1。发送开关灯测试帧后，检查 UART2 是否打印：

- `[UART1 RX] len=...`
- 接收到的十六进制字节
- APP 开关灯处理结果或校验错误信息

当前接收中断路径还会对收到的帧进行原样回传，便于使用串口工具观察回显。

### 7.3 按键验证

- 短按后检查 `KEY has been shortdown` 相关日志。
- 持续按住约 3 s 后检查长按日志。
- 若按键无响应，优先确认 `DEF_SOP20_PACKAGE` 与实际芯片封装和按键引脚是否一致。

### 7.4 Flash 验证

保持 `NVM_DEMO_TEST=1`，复位设备并检查 `0x0800FFE1` 写入前后的值。验证完成后建议恢复为：

```c
#define NVM_DEMO_TEST 0
```

避免每次启动都执行用户页擦写。

### 7.5 构建检查

在 Keil 中重新构建后，确认：

- 编译和链接无错误。
- `Objects/Project.hex`、`bin/Project.bin` 和 `bin/Project.chk` 已更新。
- 若调整 Flash 用户区布局，检查 `Project.map`，确认程序区没有覆盖 `0x0800FF80` 起始的 NVM 区域。

## 8. 已知限制与后续开发建议

- 开发板原理图中的板载 LED2 接在 PB5，当前 `LEDB_H/LEDB_L` 宏和 `APP_GpioConfig_PB5()` 已按低电平点亮逻辑直接驱动该引脚。
- 当前工程 Target 为 `PY32F003x8`，实物芯片为 `PY32F003F18P6 TSSOP20`；两者的 DFP 设备选择和存储配置需要在正式发布前按精确料号复核。
- 当前 UART2 调试代码使用 PB6(TX)/PB7(RX)，均为 TSSOP20 原理图 P2 已引出的 USART2 AF4 复用引脚。
- PB6 同时是 WS2812B 数据预留脚；当前未调用 `APP_GpioConfig_PB6()`，启用 WS2812B GPIO 输出后会与 UART2 TX 冲突。
- 原理图没有板载 WS2812B 灯珠和对应供电电路，PB6/PA1 只能作为外接 WS2812B 功能的扩展引脚，不能据此判断开发板自带灯环。
- `PY32F003xx_Project_LL/User/Src/user_ble_pair.c` 当前主要是协议说明和接口占位，完整 BLE 配网状态机、配网轮询、NetworkID 持久化和故障灯效尚未在该文件中完整实现。README 只描述当前已落地的 UART1 帧接收和 APP 开关灯处理。
- `Drivers/BSP/Src/user_bsp_ws2812b.c` 中的 `ws2812b_Init()` 当前为空。工程中已经存在 WS2812B GPIO 宏和 29 颗灯的彩虹颜色表，但完整的灯珠时序发送与显示更新链路仍需继续实现。
- Flash 用户 NVM 位于程序 Flash 的末页附近，新增或调整链接布局时必须保留该区域，避免固件覆盖掉电存储数据。
- `Delay_ms()` 是忙等待延时，适合启动阶段短延时，不建议在主循环中用于长时间业务等待。
- UART1 接收缓冲区和帧缓冲区当前大小均为 256 字节，扩展协议时需要同步评估 DMA 长度、帧尾重定位和异常帧处理。
- 时间片任务函数当前有多个空实现或演示日志，新增业务时应控制单次任务执行时间，避免阻塞其他周期任务和 UART 帧处理。

## 9. 参考资料

- [开发板原理图](<devkitdoc/PY32F003F1xP_START_V1.1_SCH(1).pdf>)
- [PY32F003 芯片资料目录](PY32F003doc/)
- [PY32F003 系列数据手册](<PY32F003doc/PY32F003系列数据手册_V2.7_yuanshi(1).pdf>)
- [PY32F003 系列参考手册](<PY32F003doc/PY32F003系列参考手册_V1.5_yuanshi(1).pdf>)
- [PY32F003 LL 样例说明（芯片资料）](<PY32F003doc/PY32F003_LL Sample Description(1).pdf>)
- [PY32F003 用户手册](Documentation/PY32F003_User_Manual.chm)
- [PY32F002A 用户手册](Documentation/PY32F002A_User_Manual.chm)
- [PY32F030 用户手册](Documentation/PY32F030_User_Manual.chm)
- [PY32F003 LL 样例说明](PY32F003_LL%20Sample%20Description.pdf)
- [现有子工程说明](PY32F003xx_Project_LL/readme.txt)
- [Keil 工程](PY32F003xx_Project_LL/MDK-ARM/Project.uvprojx)
- [构建日志](PY32F003xx_Project_LL/MDK-ARM/build_log.txt)
