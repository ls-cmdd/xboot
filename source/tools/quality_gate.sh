#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
make clean test test-linux-userspace test-tcg2 test-secureboot test-direct-linux test-hash-negative
make clean image
make test-real-fat32
make fuzz-smoke
if grep -RInE 'TODO|FIXME|placeholder' --include='*.c' --include='*.h' include core arch hardware mm storage filesystems loaders platform; then
    echo "forbidden implementation marker found" >&2
    exit 1
fi
if nm -u build/xboot.efi 2>/dev/null | grep -Ev '^$'; then
    echo "unexpected unresolved symbol in UEFI image" >&2
    exit 1
fi
printf '%s\n' 'quality gate: PASS'
