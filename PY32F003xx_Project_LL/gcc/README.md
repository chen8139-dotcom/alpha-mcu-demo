# PY32F003xx — macOS / Linux GCC 构建

在 macOS 上用 ARM GNU 工具链（`arm-none-eabi-gcc`）编译本工程，产物与
Keil MDK 构建对应（`.hex` / `.bin` / `.chk`），可直接配合
`MDK-ARM/flashtool/flash.sh`（OpenOCD + J-Link）烧录。

Keil 工程文件（`MDK-ARM/Project.uvprojx` 等）**完全未改动**，Windows 侧
继续用 MDK 构建，两边互不影响。

## 1. 环境准备（一次性）

安装 ARM GNU 工具链，二选一：

**方式 A：官方 tarball 解压（推荐，免 sudo）**

Makefile 会自动检测 `~/toolchains/arm-gnu-toolchain-*`，无需配置 PATH：

```bash
mkdir -p ~/toolchains
curl -L -o /tmp/arm-gnu.tar.xz \
  "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-darwin-arm64-arm-none-eabi.tar.xz"
tar -xJf /tmp/arm-gnu.tar.xz -C ~/toolchains && rm /tmp/arm-gnu.tar.xz
```

**方式 B：Homebrew cask（需要输入 sudo 密码的图形 pkg 安装）**

```bash
brew install --cask gcc-arm-embedded
```

其余依赖 macOS 自带 / 已有：

```bash
make --version    # /usr/bin/make
python3 --version # flashtool 与校验脚本依赖
```

## 2. 构建

```bash
cd PY32F003xx_Project_LL/gcc
make            # 编译 → build/Project.{elf,hex,bin,map} + bin/Project.chk
make flash      # 编译 + J-Link 烧录 (调用 ../MDK-ARM/flashtool/flash.sh)
make clean      # 清理
```

编译后同样会生成 Keil after-build 等价产物：
`MDK-ARM/bin/Project.bin` + `MDK-ARM/bin/Project.chk`（16 位累加和校验，
算法与 `tools/gen_bin_sum16.ps1` 一致）。

优化等级默认 `-O0`（对齐 Keil 工程 `<Optim>1`），可覆盖：

```bash
make OPT=-Os
```

## 3. 与 Keil 构建的对应关系

| 项目 | Keil (MDK-ARM) | GCC (本目录) |
|---|---|---|
| 芯片 | PY32F003x8 | 同 |
| 编译器 | ARM Compiler 5 (armcc) | arm-none-eabi-gcc |
| 启动文件 | `MDK-ARM/startup_py32f003xx.s` (armasm) | `gcc/startup_py32f003x8_gcc.s` (GNU as)，向量表 1:1 移植 |
| 链接配置 | `Objects/Project.sct` | `gcc/py32f003x8.ld` |
| 宏 | `PY32F003x8,USE_FULL_LL_DRIVER` | 同 |
| C 标准 / 优化 | C99 / -O0 | gnu99 / -O0（可 `OPT=` 覆盖） |
| after-build | fromelf→bin + PowerShell sum16 | objcopy→bin + `gen_bin_sum16.py` |

### 与 Keil 的两处有意差异

1. **Flash 可用长度收紧为 0xFF80**：应用把最后一页 Flash
   （`0x0800FF80..0x0800FFFF`）用作 NVM 数据区（见
   `User/Src/user_flash_manage.c`），链接脚本因此把程序区上限设为
   `0x0800FF80`，由链接器保证程序不会覆盖 NVM 页。Keil 的 sct 仍是整
   64K，靠人工保证。
2. **栈**：Keil 启动文件静态分配 0x400 栈区；GCC 下栈从 `_estack`
   （0x20002000）向下生长，0x400 仅作链接期余量检查。

注意：两个编译器生成的二进制**不等价**（指令选择/布局不同），功能
等价。烧录 hex 时不要混用两个环境的产物判断差异。

## 4. 文件说明

| 文件 | 作用 |
|---|---|
| `startup_py32f003x8_gcc.s` | GNU as 语法启动文件（向量表 + Reset_Handler） |
| `py32f003x8.ld` | 链接脚本（FLASH 0x08000000/0xFF80，RAM 0x20000000/0x2000） |
| `Makefile` | 构建；源文件清单与 `Project.uvprojx` 一致 |
| `gen_bin_sum16.py` | after-build 校验（Project.bin → Project.chk） |

## 5. 常见问题

- **找不到 arm-none-eabi-gcc**：确认 `brew install --cask gcc-arm-embedded`
  完成，且 `/opt/homebrew/bin` 在 PATH 中（新开终端或 `hash -r`）。
- **代码体积比 Keil 大**：`-O0` 下 GCC 产物明显大于 armcc，属正常；
  空间紧张时用 `make OPT=-Os`。
- **改了 Keil 工程里的源文件清单**（增删 .c 文件）：需要同步修改
  `Makefile` 的 `C_SOURCES`。
