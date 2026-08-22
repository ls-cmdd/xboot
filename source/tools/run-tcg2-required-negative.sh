#!/bin/bash
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
ESP=${1:?usage: run-tcg2-required-negative.sh <esp-image>}
rm -f build/qemu.tcg2.required-negative.log build/linux.tcg2.required-negative.serial.log
cp /usr/share/OVMF/OVMF_VARS_4M.fd build/OVMF_VARS.tcg2.required-negative.fd
timeout 20s qemu-system-x86_64 \
  -machine q35 -m 256M -no-reboot -display none -monitor none \
  -serial file:build/linux.tcg2.required-negative.serial.log \
  -debugcon file:build/qemu.tcg2.required-negative.log -global isa-debugcon.iobase=0x402 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=build/OVMF_VARS.tcg2.required-negative.fd \
  -drive format=raw,file="$ESP" >/dev/null 2>&1 || true
grep -a -q 'TCG2 required kernel measurement failed: not found' build/qemu.tcg2.required-negative.log
grep -a -q 'Linux artifact validation failed: not found' build/qemu.tcg2.required-negative.log
! grep -a -q 'Linux version' build/linux.tcg2.required-negative.serial.log
printf '%s\n' 'TCG2 required-policy negative test: PASS'
