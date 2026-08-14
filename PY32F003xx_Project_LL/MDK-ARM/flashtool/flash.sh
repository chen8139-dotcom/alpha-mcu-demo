#!/bin/bash
# ============================================================
#  PY32F003 J-Link 一键烧录脚本 (macOS)
#  用法: ./flash.sh [hex文件路径]
#  默认烧录 MDK-ARM/Objects/Project.hex
# ============================================================
set -e

# --- 路径配置 -------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OCD="$SCRIPT_DIR/openocd"
TCL_DIR="$SCRIPT_DIR/tcl"
HEX="${1:-$SCRIPT_DIR/../Objects/Project.hex}"
BIN="/tmp/py32_project.bin"
CFG="$SCRIPT_DIR/flash.cfg"

# --- J-Link 序列号（多仿真器时用来选中正确的那个） ------------
SN="150711039"

# --- 检查工具 -------------------------------------------------
if [ ! -x "$OCD" ]; then
    echo "错误: 找不到 openocd 二进制: $OCD" >&2
    exit 1
fi
if [ ! -f "$HEX" ]; then
    echo "错误: 找不到固件 hex 文件: $HEX" >&2
    echo "请先编译工程, 或指定 hex 路径" >&2
    exit 1
fi

# --- hex 转 bin (Intel HEX -> 二进制, 从 0x08000000 起始) -------
python3 - "$HEX" "$BIN" <<'PYEOF'
import sys
hexpath, binpath = sys.argv[1], sys.argv[2]
FLASH_BASE = 0x08000000
data = {}
for line in open(hexpath):
    line = line.strip()
    if not line.startswith(":"):
        continue
    n = int(line[1:3], 16)
    addr = int(line[3:7], 16)
    rectype = int(line[7:9], 16)
    payload = bytes(int(line[9+2*i:11+2*i], 16) for i in range(n))
    if rectype == 4:
        pass  # 本工程只有 0x0800 段
    elif rectype == 0:
        for i, b in enumerate(payload):
            data[FLASH_BASE + addr + i] = b
min_a = min(data)
max_a = max(data)
size = (max_a - FLASH_BASE + 1 + 0xFF) & ~0xFF
buf = bytearray(size)
for a, b in data.items():
    buf[a - FLASH_BASE] = b
open(binpath, "wb").write(buf)
print(f"hex -> bin: {len(buf)} 字节 (0x{min_a:08X}..0x{max_a:08X})")
PYEOF

# --- 生成烧录配置 ---------------------------------------------
cat > "$CFG" <<EOF
# OpenOCD 烧录: J-Link SWD + PY32F003x8 (64K flash)
source [find interface/jlink.cfg]
adapter serial $SN
transport select swd
set FLASH_SIZE 0x10000
set WORKAREASIZE 0x1000
source [find target/py32f0xx.cfg]

init
reset halt
flash write_image erase $BIN 0x08000000 bin
verify_image $BIN 0x08000000
reset run
shutdown
EOF

# --- 烧录 ------------------------------------------------------
echo "==> 通过 J-Link (SN $SN) 烧录 PY32F003x8 ..."
"$OCD" -s "$TCL_DIR" -f "$CFG"
echo "==> 烧录完成"
