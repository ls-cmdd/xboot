# Initramfs parser research notes

## Source reviewed

The Linux kernel documentation at https://docs.kernel.org/driver-api/early-userspace/buffer-format.html defines initramfs buffers around newc or crc CPIO archives. A CPIO file is aligned to four bytes, has a fixed 110-byte hexadecimal-ASCII header, followed by a NUL-terminated filename and aligned data. The `TRAILER!!!` filename is the end-of-archive marker; compressed archives are a separate concern and are not treated as raw newc data.

## Implementation implications

XBoot's first initramfs milestone will inspect uncompressed newc/070701 and crc/070702 members. It will validate all hexadecimal fields, namesize/filesize bounds, four-byte alignment, regular-file and trailer semantics, and optional crc data sums. It will expose bounded entry metadata only; decompression and filesystem extraction remain separate capabilities.
