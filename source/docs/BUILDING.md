# Building XBoot

The supported development host is Ubuntu with Clang, LLD, QEMU, OVMF, `mtools`, and `dosfstools`. The Makefile fails on compiler warnings in the current phase so ABI and layout mistakes are visible during development.

The UEFI binary is installed at the removable-media path `EFI/BOOT/BOOTX64.EFI` inside a FAT32 ESP image. The image is generated from scratch for each build and the QEMU test uses a copied OVMF variable store under `build/`.

QEMU output is sent to the ISA debug console at I/O port `0x402`. The XBoot logger also uses the UEFI text output protocol when available. The test harness asserts the entry banner, system-table validation, and memory-map discovery markers; the harness does not infer Linux support from these markers.
