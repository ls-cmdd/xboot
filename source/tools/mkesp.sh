#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 5 ]]; then
    printf 'usage: %s <xboot.efi> <esp.img> [kernel] [initrd] [config]\n' "$0" >&2
    exit 2
fi

EFI_BINARY=$1
IMAGE=$2
KERNEL=${3:-}
INITRD=${4:-}
CONFIG=${5:-}

mkdir -p "$(dirname "$IMAGE")"
rm -f "$IMAGE"
truncate -s 128M "$IMAGE"
mkfs.fat -F 32 -n XBOOT "$IMAGE" >/dev/null
mmd -i "$IMAGE" ::/EFI
mmd -i "$IMAGE" ::/EFI/BOOT
mcopy -i "$IMAGE" "$EFI_BINARY" ::/EFI/BOOT/BOOTX64.EFI
if [[ -n "$KERNEL" || -n "$INITRD" || -n "$CONFIG" ]]; then
    mmd -i "$IMAGE" ::/EFI/XBOOT
fi
if [[ -n "$KERNEL" ]]; then
    mcopy -i "$IMAGE" "$KERNEL" ::/EFI/XBOOT/vmlinuz.efi
fi
if [[ -n "$INITRD" ]]; then
    mcopy -i "$IMAGE" "$INITRD" ::/EFI/XBOOT/initrd.img
fi
if [[ -n "$CONFIG" ]]; then
    mcopy -i "$IMAGE" "$CONFIG" ::/EFI/XBOOT/xboot.conf
fi
