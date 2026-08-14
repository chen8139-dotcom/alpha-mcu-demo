# PY32F003 J-Link 烧录工具 (macOS)

在 macOS 上通过 J-Link 给 PY32F003x8 烧录固件的一键工具。

## 用法

```bash
./flash.sh                          # 烧录 ../Objects/Project.hex (Keil 编译产物)
./flash.sh 其他固件.hex              # 烧录指定 hex
```

## 原理

- 自带编译好的 OpenOCD（基于 BuQiEmbedded/openocd-buqi，含 Puya puyaf0x 闪存驱动）
- J-Link 作为 SWD 适配器（OpenOCD 的 jlink 驱动，需要 libjaylink）
- 自动完成：hex→bin 转换 → 擦除 → 写入 → 校验 → 复位运行

## 注意事项

- J-Link 序列号在 `flash.sh` 中 `SN="150711039"`，如有多个仿真器请改成你的
- 目标芯片固定为 PY32F003x8（64KB Flash），其他型号改 `FLASH_SIZE` 变量
- 擦除/编程期间 "Failed to read memory" 日志是正常的：PY32 的 DAP 在闪存操作期间
  会暂时失联，驱动会自动重试，最终校验通过即烧录成功

## 文件说明

| 文件 | 说明 |
|------|------|
| `openocd` | OpenOCD 二进制（含 puyaf0x 驱动） |
| `tcl/` | OpenOCD 脚本（interface/jlink.cfg + target/py32f0xx.cfg） |
| `flash.sh` | 一键烧录脚本 |
| `flash.cfg` | 生成的烧录配置（自动产生，无需编辑） |

## 依赖

- macOS 需安装 libjaylink: `brew install libjaylink`
- 无需 SEGGER 官方软件（但 J-Link 硬件固件需要已更新）
