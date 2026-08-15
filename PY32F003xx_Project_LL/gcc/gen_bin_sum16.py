#!/usr/bin/env python3
"""After Build: 16-bit additive checksum of Project.bin -> Project.chk (XXXX + CRLF)

macOS port of MDK-ARM/tools/gen_bin_sum16.ps1, byte-for-byte identical output:
    s = (sum of all bin bytes) & 0xFFFF
    Project.chk = 4-digit uppercase hex + CRLF
"""
import sys
from pathlib import Path

if __name__ == "__main__":
    # 脚本位于 gcc/，bin 目录与 Keil 的 MDK-ARM/bin 一致放在 MDK-ARM/bin
    root = Path(__file__).resolve().parent.parent / "MDK-ARM"
    bin_path = root / "bin" / "Project.bin"
    chk_path = root / "bin" / "Project.chk"

    try:
        data = bin_path.read_bytes()
    except FileNotFoundError:
        print(f"gen_bin_sum16: missing {bin_path}", file=sys.stderr)
        sys.exit(1)

    s = sum(data) & 0xFFFF
    chk_path.write_bytes(f"{s:04X}\r\n".encode("ascii"))
    print(f"BIN_SUM16={s:04X}")
