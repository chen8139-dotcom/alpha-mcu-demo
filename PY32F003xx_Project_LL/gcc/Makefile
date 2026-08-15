# ============================================================
#  PY32F003xx_Project_LL - macOS / Linux GCC 构建脚本
#
#  与 Keil MDK-ARM 工程等价的源文件清单、宏定义、包含路径：
#    - 芯片:        PY32F003x8 (Cortex-M0+, 64K Flash / 8K SRAM)
#    - 宏:          PY32F003x8, USE_FULL_LL_DRIVER
#    - C 标准:      gnu99, -O0 (与 Keil AC5 设置一致, 可用 OPT=-Os 覆盖)
#  产物: build/Project.elf/.hex/.bin/.map + MDK-ARM/bin/Project.bin/.chk
#
#  常用命令:
#    make          编译
#    make flash    编译 + 通过 J-Link 烧录 (调用 MDK-ARM/flashtool/flash.sh)
#    make clean    清理
# ============================================================

# --- 工具链 --------------------------------------------------
# 优先用 PATH 中的 arm-none-eabi-gcc (brew pkg / 手动安装);
# 若 PATH 中没有, 自动使用官方 tarball 解压版 ~/toolchains/arm-gnu-toolchain-*
# 注: 不用 export PATH 注入, macOS 自带 make 3.81 直接 exec 时会忽略它
TARGET   := arm-none-eabi
TOOLCHAIN_BIN := $(firstword $(wildcard $(HOME)/toolchains/arm-gnu-toolchain-*/bin))
ifeq ($(shell command -v $(TARGET)-gcc 2>/dev/null),)
  ifneq ($(TOOLCHAIN_BIN),)
    TC_PREFIX := $(TOOLCHAIN_BIN)/
  endif
endif

CC       := $(TC_PREFIX)$(TARGET)-gcc
AS       := $(TC_PREFIX)$(TARGET)-gcc
CP       := $(TC_PREFIX)$(TARGET)-objcopy
SZ       := $(TC_PREFIX)$(TARGET)-size

# --- 目录 ----------------------------------------------------
BUILD_DIR := build
PROJ      := ..
DRIVERS   := ../..

# --- 芯片与编译选项（对齐 Project.uvprojx）-------------------
CPU       := -mcpu=cortex-m0plus -mthumb
DEFS      := -DPY32F003x8 -DUSE_FULL_LL_DRIVER
# Keil: <Optim>1 = -O0, <uC99>1 = C99; gnu99 允许 GNU 扩展, 兼容性最好
OPT       ?= -O0
CSTD      := -std=gnu99

INC := \
  -I$(DRIVERS)/Drivers/CMSIS/Include \
  -I$(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Inc \
  -I$(PROJ)/Inc \
  -I$(DRIVERS)/Drivers/CMSIS/Device/PY32F0xx/Include \
  -I$(PROJ)/User/Inc \
  -I$(DRIVERS)/Drivers/BSP/Inc

# --- 源文件（与 Keil 工程源组一一对应）-----------------------
STARTUP   := startup_py32f003x8_gcc.s

C_SOURCES := \
  $(PROJ)/Src/system_py32f0xx.c \
  $(PROJ)/Src/main.c \
  $(PROJ)/Src/py32f0xx_it.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_utils.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_adc.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_tim.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_exti.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_gpio.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_usart.c \
  $(DRIVERS)/Drivers/PY32F0xx_HAL_Driver/Src/py32f0xx_ll_rcc.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_adc.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_dma.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_gpio.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_key.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_timer.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_uart1.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_uart2.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_ws2812b.c \
  $(DRIVERS)/Drivers/BSP/Src/user_bsp_flash.c \
  $(PROJ)/User/Src/user_common.c \
  $(PROJ)/User/Src/user_task.c \
  $(PROJ)/User/Src/user_light_pattern.c \
  $(PROJ)/User/Src/user_ble_pair.c \
  $(PROJ)/User/Src/user_ble_uart.c \
  $(PROJ)/User/Src/user_flash_manage.c

# --- 编译/链接 flags -----------------------------------------
COMMON_FLAGS := $(CPU) $(DEFS) $(INC)

CFLAGS := $(COMMON_FLAGS) $(CSTD) $(OPT) \
  -ffunction-sections -fdata-sections \
  -Wall -fno-common

ASFLAGS := $(CPU) $(DEFS) -x assembler-with-cpp

LDSCRIPT := py32f003x8.ld
LDFLAGS  := $(CPU) -specs=nano.specs -specs=nosys.specs \
  -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/Project.map,--cref \
  -Wl,--gc-sections \
  -Wl,--start-group -lc -lm -Wl,--end-group

# --- 对象文件 ------------------------------------------------
OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(STARTUP:.s=.o))
vpath %.s $(dir $(STARTUP))

# --- 目标 ----------------------------------------------------
ELF  := $(BUILD_DIR)/Project.elf
HEX  := $(BUILD_DIR)/Project.hex
BIN  := $(BUILD_DIR)/Project.bin
KEIL_BIN_DIR := $(PROJ)/MDK-ARM/bin

.PHONY: all flash clean

all: $(HEX) $(BIN) post-build
	$(SZ) $(ELF)

$(ELF): $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(HEX): $(ELF)
	$(CP) -O ihex $< $@

$(BIN): $(ELF)
	$(CP) -O binary -S $< $@

# After-build: 与 Keil 一致, 生成 MDK-ARM/bin/Project.bin + Project.chk
post-build: $(BIN)
	@mkdir -p $(KEIL_BIN_DIR)
	@cp $(BIN) $(KEIL_BIN_DIR)/Project.bin
	@python3 gen_bin_sum16.py

flash: all
	$(PROJ)/MDK-ARM/flashtool/flash.sh $(HEX)

clean:
	rm -rf $(BUILD_DIR)

# --- 依赖关系 ------------------------------------------------
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $@
