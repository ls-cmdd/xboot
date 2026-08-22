#!/bin/bash
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
ESP=${1:?usage: run-tcg2-test.sh <esp-image>}
STATE=build/tcg2-test-state
SOCK=build/tcg2-test.sock
rm -rf "$STATE" "$SOCK" build/qemu.tcg2.test.log build/linux.tcg2.test.serial.log \
  build/qemu.tcg2.test.stderr build/OVMF_VARS.tcg2.test.fd
mkdir -p "$STATE"
cp /usr/share/OVMF/OVMF_VARS_4M.fd build/OVMF_VARS.tcg2.test.fd
swtpm socket --tpm2 --tpmstate dir="$STATE" \
  --ctrl type=unixio,path="$SOCK" --flags startup-clear \
  --pid file="$STATE/swtpm.pid" --log level=20,file="$STATE/swtpm.log" --daemon
for _ in $(seq 1 10); do
    [ -S "$SOCK" ] && break
    sleep 1
done
[ -S "$SOCK" ]
TPM_PID=$(cat "$STATE/swtpm.pid")
cleanup() {
    kill "$TPM_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM
timeout 30s qemu-system-x86_64 \
  -machine q35 -m 256M -no-reboot -display none -monitor none \
  -serial file:build/linux.tcg2.test.serial.log \
  -debugcon file:build/qemu.tcg2.test.log -global isa-debugcon.iobase=0x402 \
  -chardev socket,id=chrtpm,path="$SOCK" \
  -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=build/OVMF_VARS.tcg2.test.fd \
  -drive format=raw,file="$ESP" >/dev/null 2>&1 || true
printf '%s\n' 'TCG2 debug markers:'
grep -a -E 'TCG2 measured Linux (kernel|initrd)' build/qemu.tcg2.test.log
printf '%s\n' 'Linux TPM PCR marker:'
grep -a -E 'XBOOT_TPM_PCR4_BEGIN|XBOOT_TPM_PCR4_END' build/linux.tcg2.test.serial.log
if grep -a -q 'XBOOT_TPM_PCR4_UNAVAILABLE' build/linux.tcg2.test.serial.log; then
    echo 'TPM PCR4 was unavailable inside Linux' >&2
    exit 1
fi
PCR_LINE=$(awk '/XBOOT_TPM_PCR4_BEGIN/{capture=1; next} capture && /^[[:space:]]*[0-9a-fA-F]/{print; exit}' build/linux.tcg2.test.serial.log)
if [ -z "$PCR_LINE" ] || ! printf '%s\n' "$PCR_LINE" | grep -q '[1-9a-fA-F]'; then
    echo 'TPM PCR4 output was empty or all zero' >&2
    exit 1
fi
printf 'PCR4: %s\n' "$PCR_LINE"
