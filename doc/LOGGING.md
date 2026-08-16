# 项目日志配置与调试指南

本文档是本项目日志串口配置、日志开关、日志来源和调试方法的统一说明。
后续新增或修改日志输出时，应同步维护本文档。

## 1. 快速上手

### 1.1 调试串口

当前调试日志使用 `USART2`：

| 项目 | 配置 |
| --- | --- |
| MCU TX | PB6 |
| MCU RX | PB7 |
| 复用功能 | USART2 AF4 |
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | None |
| 硬件流控 | None |
| 输出方式 | `printf()` 重定向到 UART2 |

USB-UART 转换器连接方式：

```text
MCU PB6 (TX)  ->  USB-UART RX
MCU PB7 (RX)  <-  USB-UART TX       # 当前仅完成硬件初始化
MCU GND       ->  USB-UART GND
```

UART 电平必须与 MCU 的 3.3 V 逻辑兼容。复位设备后，日志应从 PB6 输出。

### 1.2 预期启动输出

当前默认配置下，启动阶段至少会看到：

```text
track_xx user_sys_init_1_1...
hello PY32F003
```

板载 LED（PB5）不会再执行独立的 1ms 启动翻转。LED 由 Beacon Mesh 状态机统一管理：上电前 3 秒每 500ms 翻转，之后 Leader 常亮、Follower 仅在有效 Leader 心跳时亮 100ms，未连接模组和 Candidate 状态灭灯。

由于 `NVM_DEMO_TEST=1`，启动阶段还会输出用户 Flash Dump、读写结果等较多内容。
进入主循环后，`Task_512ms()` 当前会持续输出周期日志。

## 2. 串口资源分工

| 串口 | 引脚 | 复用功能 | 用途 |
| --- | --- | --- | --- |
| UART1 / USART1 | PA2(TX)、PA3(RX) | AF1 | 蓝牙模组/业务协议通信 |
| UART2 / USART2 | PB6(TX)、PB7(RX) | AF4 | 调试日志输出 |

UART1 的 DMA + IDLE 接收链路和 UART2 的日志输出链路相互独立。UART1 收到数据后，
应用层会将接收帧信息通过 UART2 打印出来。

相关配置位置：

- UART2 引脚和串口参数：`Drivers/BSP/Inc/user_bsp_uart2.h`
- UART2 初始化和 `printf` 重定向：`Drivers/BSP/Src/user_bsp_uart2.c`
- UART1 GPIO、DMA 和 IDLE 接收：`Drivers/BSP/Src/user_bsp_uart1.c`
- 系统启动初始化：`PY32F003xx_Project_LL/User/Src/user_common.c`

## 3. 日志输出架构

当前日志链路如下：

```text
业务代码 printf(...)
        |
        +-- Keil/ARMCC: fputc()
        +-- IAR:        putchar()
        +-- GCC:        __io_putchar() / _write()
        |
        +-- LL_USART_TransmitData8(USART2, ch)
        +-- 轮询等待 TC
        |
        +-- PB6 (USART2_TX)
```

UART2 使用阻塞式发送。每个字符发送后等待 USART 的 `TC` 标志，当前没有使用 UART2
TX DMA、TX 中断或 RAM 环形缓冲区。

### 3.1 编译器重定向入口

`user_bsp_uart2.c` 根据编译器提供以下入口：

| 编译器 | 重定向入口 |
| --- | --- |
| Keil ARMCC | `fputc()`，接收使用 `fgetc()` |
| IAR | `putchar()` |
| GCC | `__io_putchar()` 和 `_write()` |

当前 UART2 RX 只配置了 GPIO 和 USART 方向，工程没有 UART2 接收中断、DMA 或调试命令
解析流程。不要在主循环中直接调用 `fgetc()`，否则会阻塞等待输入。

## 4. 日志开关与当前默认值

| 宏 | 当前默认值 | 作用 | 当前状态 |
| --- | ---: | --- | --- |
| `DEF_Develop_Release` | `1` | 研发/发行阶段条件日志 | 生效 |
| `APP_BLE_DEBUG_LOG` | `1U` | BLE/业务日志开关 | 生效 |
| `APP_VERBOSE_UART1_RX` | `1U` | UART1 原始接收帧十六进制日志 | 生效 |
| `APP_TASK_512MS_WOS_LOG` | `0U` | 预留的 512 ms WOS 日志开关 | 当前 `Task_512ms()` 日志未使用此宏 |
| `NVM_DEMO_TEST` | `1` | 启动时执行 Flash Dump/读写演示 | 生效，会产生大量启动日志 |
| `USER_LED_TEST` | `0` | 独立 LED 启动翻转测试 | 已关闭；LED 由 Beacon Mesh 状态机管理 |

主要配置文件：

- `PY32F003xx_Project_LL/User/Inc/user_common.h`
- `PY32F003xx_Project_LL/User/Inc/user_board_cfg.h`

注意：`APP_BLE_DEBUG_LOG` 和 `APP_VERBOSE_UART1_RX` 使用 `#ifndef`，可以由工程编译
宏或上层配置覆盖。新增日志开关时，应同时记录默认值、作用范围和发行版本行为。

## 5. 当前日志清单

### 5.1 系统初始化日志

| 输出示例 | 触发时机 | 条件 | 来源 |
| --- | --- | --- | --- |
| `track_xx user_sys_init_1_1...` | UART2 初始化完成后 | 无条件 | `PY32F003xx_Project_LL/User/Src/user_common.c` |
| `hello PY32F003` | `main()` 完成系统初始化后 | 无条件 | `PY32F003xx_Project_LL/Src/main.c` |

### 5.2 Flash/NVM 演示日志

当 `NVM_DEMO_TEST=1` 时，`main()` 调用 `NVM_DemoRealScenario()`，输出内容包括：

- 用户 Flash 页起止地址。
- 写入前的 Flash 内容。
- `0x0800FFE1` 的演示字节读取值。
- 写入 `0xED` 的结果。
- 写入后的 Flash Dump。

来源：`PY32F003xx_Project_LL/User/Src/user_flash_manage.c`。

该演示会产生较多阻塞式日志，并且会执行 Flash 写操作。长期运行或正式固件中应根据
需要关闭 `NVM_DEMO_TEST`。

### 5.3 UART1 接收日志

UART1 通过 DMA 接收并由 IDLE 中断切帧，应用层在主循环中处理帧。启用
`APP_BLE_DEBUG_LOG=1` 和 `APP_VERBOSE_UART1_RX=1` 时，UART2 会输出：

```text
[UART1 RX] len=...
```

以及接收到的十六进制字节。帧异常、帧头重定位、帧尾缺失、校验错误和未知命令等
情况还会输出对应的 `[UART1 RX]` 或 `[BLE]` 日志。

来源：`PY32F003xx_Project_LL/User/Src/user_ble_uart.c`。

### 5.4 Beacon Mesh 状态与遥控器日志

UART1 协议层在主循环中输出以下业务日志：

```text
[BLE] module ready
[BLE] MAC/ElectionID=1A 09 E2 49 8F 34
[MESH] INIT -> FOLLOWER reason=MAC_READY
[MESH] FOLLOWER -> CANDIDATE reason=LEADER_TIMEOUT
[MESH] CANDIDATE -> LEADER reason=CANDIDATE_DELAY
[MESH] TX LEADER_ADV seq=1 network=0x1234 flags=0x00
[REMOTE] source=遥控器 addr=0x1115 cmd=0x0A type=短按 para=0x00 rand=0x00 check=OK
```

业务日志由 `APP_BLE_DEBUG_LOG` 控制，原始 UART1 十六进制日志由
`APP_VERBOSE_UART1_RX` 控制。当前固定 `NetworkID=0x1234`，收到 `Cmd=0xFF`
只记录“configuration ignored”，不会修改或写入 NetworkID。

UART1 中断只负责锁存 DMA 帧，不执行 `printf()` 或阻塞式发送。来源：
`PY32F003xx_Project_LL/User/Src/user_ble_uart.c` 和
`Drivers/BSP/Src/user_bsp_uart1.c`。

### 5.5 按键和业务日志

研发模式 `DEF_Develop_Release=1` 时：

- 按键短按输出 `track_xx KEY has been shortdown`。
- 按键长按输出 `[PAIR] long press, factory re-provision`。
- APP 开灯、关灯、非法动作和校验错误输出 `[BLE]` 日志。

来源：

- `PY32F003xx_Project_LL/User/Src/user_task.c`
- `PY32F003xx_Project_LL/User/Src/user_ble_uart.c`

### 5.6 周期任务日志

`Task_512ms()` 由 `APP_TASK_512MS_WOS_LOG` 控制，默认值为 `0U`。开启后输出：

```text
track_xx Task_512ms is running...
```

该日志约每 512 ms 输出一次。

### 5.7 时间同步辅助日志

`APP_SyncLogWosTicks()` 可以输出 WOS tick、十六进制 tick 和运行时间，受
`DEF_Develop_Release && APP_BLE_DEBUG_LOG` 控制。它是辅助接口，当前是否产生输出取决于
业务代码是否调用该函数。

## 6. 调试排查

### 6.1 完全没有日志

按以下顺序检查：

1. USB-UART 的 RX 是否接到 MCU 的 PB6，而不是接到 PB7。
2. USB-UART 与 MCU 是否共地。
3. 串口参数是否为 `115200 8N1`，校验位为 None。
4. 是否使用 3.3 V TTL 串口，而不是 RS-232 电平。
5. 固件是否确实执行了 `BSP_USART_Config()`。
6. 读取 `Drivers/BSP/Inc/user_bsp_uart2.h`，确认 TX/RX 为 PB6/PB7、AF4。

### 6.2 输出乱码

优先检查：

- 波特率是否为 115200。
- 数据位、停止位和校验位是否为 8N1。
- `SystemCoreClock` 是否与实际系统时钟一致。
- USB-UART 电平和地线是否正常。

### 6.3 UART1 能收包但没有业务日志

检查：

- `APP_BLE_DEBUG_LOG` 是否为 `1U`。
- `APP_VERBOSE_UART1_RX` 是否为 `1U`。
- `DEF_Develop_Release` 是否为 `1`。
- 主循环是否持续调用 `APP_HandleBleUartFrame()`。
- UART2 的 PB6 是否连接到串口工具 RX。

### 6.4 日志过多或影响任务运行

当前日志为阻塞式发送，以下输出尤其容易占用主循环：

- Flash 整页 Dump。
- UART1 原始帧十六进制打印。
- 512 ms 周期日志。
- 高频任务中的循环日志。

调试实时性问题时，应优先关闭 `NVM_DEMO_TEST`、详细 UART1 RX 日志和不必要的周期
日志，而不是在中断中继续增加 `printf()`。

## 7. 长期维护规范

### 7.1 新增日志

每条新增日志至少应明确：

- 模块或功能名称。
- 触发条件和触发频率。
- 默认是否开启。
- 对应的编译开关。
- 是否允许在任务、中断或错误处理上下文中调用。
- 典型输出示例。

### 7.2 日志上下文约束

- 普通日志只允许在主循环或任务上下文中使用。
- 不要在 USART、DMA、定时器或 GPIO 中断中直接调用 `printf()`。
- 不要在中断中调用会等待 `TC` 的阻塞式发送接口。
- 若后续需要中断日志，应先引入无阻塞缓存、日志丢弃策略和故障上下文保护。

### 7.3 配置和硬件变更

修改以下内容时必须同步更新本文档：

- UART2 TX/RX 引脚或 AF。
- 波特率、数据格式或硬件流控。
- 日志宏名称、默认值或作用范围。
- 日志输出格式和模块前缀。
- 日志发送机制，例如改为 TX 中断或 DMA。

## 8. 当前限制与后续演进方向

当前日志系统适合低频研发调试，但还不是完整的高可靠日志框架：

- 发送为阻塞式轮询。
- 没有统一的 `ERROR/WARN/INFO/DEBUG` 日志等级。
- 没有统一时间戳、模块标签和格式规范。
- 没有 TX 环形缓冲区、TX 中断或 DMA。
- 没有日志溢出、丢包和限频统计。
- UART2 RX 尚未形成调试命令通道。
- PB6 同时是 WS2812B 数据预留脚；启用 WS2812B GPIO 输出时会与 UART2 TX 冲突。
- 尚未建立 HardFault、复位原因和异常现场日志机制。

后续如果日志量或实时性要求提高，建议按以下顺序演进：

1. 先统一 `LOGE/LOGW/LOGI/LOGD` 接口和编译开关。
2. 再增加统一模块前缀、时间戳和日志等级。
3. 将 UART2 发送改为环形缓冲区 + TX 中断或 DMA。
4. 增加缓冲区溢出统计、限频和发行版本裁剪。
5. 最后再增加 UART2 调试命令、断言和故障现场保存。
