# XBoot performance and compatibility baseline

قيس هذا baseline في sandbox على release-candidate الحالي بعد اكتمال مسار Linux EFI stub. النتائج تتأثر بالـ VM والـ filesystem، ولذلك تُستخدم للمقارنة بين builds في البيئة نفسها وليست ادعاءً لأداء hardware عام.

| Artifact / operation | Measurement |
|---|---:|
| `make clean host-test` wall time | 1.384 s |
| `make clean host-test` user CPU | 0.954 s |
| `make clean host-test` system CPU | 0.429 s |
| `make all` wall time after clean host build | 0.418 s |
| `build/xboot.efi` size | 60,416 bytes |
| `build/xboot.efi` SHA-256 | `61aa671d0c871fcc0f160fcb1d47608d6486c9d7790b215b07d5d34f31765eb8` |
| QEMU kernel-start test memory | 512 MiB |
| QEMU userspace proof memory | 256 MiB |

يستخدم `make test` QEMU/OVMF مع Ubuntu kernel/initrd حقيقيين ويلتقط `Linux version` في serial log. ويستخدم `make test-linux-userspace` initramfs مصغراً مبنياً من BusyBox static ويثبت markers `/init`. لم تُقَس هنا مدة kernel boot الكاملة لأن الاختبار ينتهي بمهلة مقصودة بعد الوصول إلى Linux/userspace.

يغطي compatibility evidence الحالي Clang C11 host builds، freestanding x86_64 UEFI PE/COFF linking، وOVMF/QEMU execution. ولا يثبت توافق physical hardware أو BIOS أو ARM64/RISC-V، ولا تشغيل PCI/NVMe/AHCI/USB drivers، أو Secure Boot signature verification، أو TPM measurement، أو direct manual Linux boot-protocol handoff.
