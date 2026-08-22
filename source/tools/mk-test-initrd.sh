#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'usage: %s <output-initrd>\n' "$0" >&2
    exit 2
fi

OUTPUT=$1
ROOT=$(mktemp -d)
trap 'rm -rf "$ROOT"' EXIT
mkdir -p "$ROOT/bin" "$ROOT/dev" "$ROOT/proc" "$ROOT/sys" "$ROOT/tmp"
cp /bin/busybox "$ROOT/bin/busybox"
ln -s busybox "$ROOT/bin/sh"
ln -s busybox "$ROOT/bin/echo"
ln -s busybox "$ROOT/bin/mount"
ln -s busybox "$ROOT/bin/sleep"
ln -s busybox "$ROOT/bin/cat"
cat > "$ROOT/init" <<'INIT'
#!/bin/sh
/bin/echo XBOOT_LINUX_USERSPACE_OK
/bin/echo XBOOT_INITRAMFS_ROOT_READY
/bin/mount -t devtmpfs devtmpfs /dev 2>/dev/null || true
/bin/mount -t proc proc /proc 2>/dev/null || true
/bin/mount -t sysfs sysfs /sys 2>/dev/null || true
if [ -r /sys/class/tpm/tpm0/pcr-sha256/4 ]; then
    /bin/echo XBOOT_TPM_PCR4_BEGIN
    /bin/cat /sys/class/tpm/tpm0/pcr-sha256/4
    /bin/echo XBOOT_TPM_PCR4_END
else
    /bin/echo XBOOT_TPM_PCR4_UNAVAILABLE
fi
while true; do /bin/sleep 1; done
INIT
chmod 0755 "$ROOT/init"
(
    cd "$ROOT"
    find . -print0 | cpio --null --quiet -o --format=newc
) | gzip -9 > "$OUTPUT"
