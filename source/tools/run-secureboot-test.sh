#!/bin/bash
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
KEY=build/secureboot/snakeoil.key.pem
CERT=/usr/share/ovmf/PkKek-1-snakeoil.pem
rm -rf build/secureboot
mkdir -p build/secureboot
openssl pkey -in /usr/share/ovmf/PkKek-1-snakeoil.key -passin pass:snakeoil -out "$KEY"
sbsign --key "$KEY" --cert "$CERT" --output build/secureboot/xboot.signed.efi build/xboot.efi >/dev/null 2>&1
sbsign --key "$KEY" --cert "$CERT" --output build/secureboot/vmlinuz.signed assets/vmlinuz >/dev/null 2>&1
sbverify --cert "$CERT" build/secureboot/xboot.signed.efi >/dev/null
sbverify --cert "$CERT" build/secureboot/vmlinuz.signed >/dev/null
./tools/mkesp.sh build/secureboot/xboot.signed.efi build/secureboot/secure-esp.img \
  build/secureboot/vmlinuz.signed assets/initrd.img config/xboot-secureboot-required.conf >/dev/null
cp /usr/share/OVMF/OVMF_VARS_4M.snakeoil.fd build/secureboot/OVMF_VARS.fd
timeout 60s qemu-system-x86_64 \
  -machine q35,smm=on -m 512M -no-reboot -display none -monitor none \
  -serial file:build/secureboot/linux.serial.log \
  -debugcon file:build/secureboot/qemu.log -global isa-debugcon.iobase=0x402 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.snakeoil.fd \
  -drive if=pflash,format=raw,file=build/secureboot/OVMF_VARS.fd \
  -drive format=raw,file=build/secureboot/secure-esp.img >/dev/null 2>&1 || true
grep -a -q 'Secure Boot enabled; authenticating Linux image through UEFI' build/secureboot/qemu.log
grep -a -q 'Secure Boot firmware authentication passed' build/secureboot/qemu.log
grep -a -q 'Starting Linux through direct 64-bit boot protocol' build/secureboot/qemu.log
grep -a -q 'Linux version' build/secureboot/linux.serial.log
grep -a -q 'Run /init' build/secureboot/linux.serial.log
printf '%s\n' 'Secure Boot signed direct test: PASS'
rm -f build/secureboot/secure-unsigned-esp.img build/secureboot/OVMF_UNSIGNED_VARS.fd \
  build/secureboot/unsigned.qemu.log build/secureboot/unsigned.linux.log
./tools/mkesp.sh build/secureboot/xboot.signed.efi build/secureboot/secure-unsigned-esp.img \
  assets/vmlinuz assets/initrd.img config/xboot.conf >/dev/null
cp /usr/share/OVMF/OVMF_VARS_4M.snakeoil.fd build/secureboot/OVMF_UNSIGNED_VARS.fd
timeout 30s qemu-system-x86_64 \
  -machine q35,smm=on -m 512M -no-reboot -display none -monitor none \
  -serial file:build/secureboot/unsigned.linux.log \
  -debugcon file:build/secureboot/unsigned.qemu.log -global isa-debugcon.iobase=0x402 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.snakeoil.fd \
  -drive if=pflash,format=raw,file=build/secureboot/OVMF_UNSIGNED_VARS.fd \
  -drive format=raw,file=build/secureboot/secure-unsigned-esp.img >/dev/null 2>&1 || true
grep -a -q 'Secure Boot image authentication failed' build/secureboot/unsigned.qemu.log
! grep -a -q 'Linux version' build/secureboot/unsigned.linux.log
printf '%s\n' 'Secure Boot unsigned-image rejection: PASS'
