# XBoot SBOM and dependency inventory

هذا الملف يسجل مكونات release-candidate المستخدمة في البناء أو الاختبار. لا يعني وجود أداة اختبار أنها dependency runtime داخل `xboot.efi`.

| Component | Version observed | Role | Runtime in XBoot |
|---|---|---|---|
| Clang | Ubuntu clang 18.1.3 | Freestanding C compilation | No |
| LLD | Ubuntu LLD 18.1.3 | PE/COFF EFI linking | No |
| QEMU | 8.2.2 | UEFI/Linux integration test | No |
| OVMF | 2024.02-2ubuntu0.9 | UEFI firmware test image | No |
| dosfstools | 4.2-1.1build1 | FAT32 ESP creation | No |
| mtools | 4.0.43-1build1 | FAT32 image population | No |
| BusyBox static | 1.36.1-6ubuntu3.1 | Controlled initramfs userspace test only | No |

XBoot itself is freestanding C/UEFI PE/COFF and does not link against GRUB, U-Boot, Limine, systemd-boot, rEFInd, libc, or a runtime package manager. Linux kernel and initrd are test inputs selected from the local environment; their hashes and local-artifact policy are recorded in `assets/README.md`.

This inventory is an environment-specific SBOM fragment, not a signed SPDX or CycloneDX attestation. A production release should generate a signed machine-readable SBOM together with compiler provenance, source revision, firmware image provenance, and reproducible-build comparison.
