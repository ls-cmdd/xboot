# XBoot compatibility matrix

هذه المصفوفة تسجل الأدلة الموجودة فعلياً في release-candidate الحالي. **PASS** يعني execution witness محفوظ في الاختبار المحدد، وليس ضماناً عاماً لكل الأجهزة أو الإصدارات.

| Dimension | Configuration | Result | Evidence / boundary |
|---|---|---|---|
| Architecture | x86_64 | PASS | freestanding EFI PE/COFF build وQEMU CPUID path |
| Firmware | OVMF 2024.02، QEMU `q35`، 512MiB | PASS | `make test`؛ EFI-stub path يطبع Linux version |
| Firmware | OVMF 2024.02، QEMU `q35`، 256MiB | PASS for EFI-stub controlled initramfs | `make test-linux-userspace` يصل `/init` |
| Firmware | OVMF 2024.02، QEMU `q35`، 512MiB direct | PASS | `make test-direct-linux` ينفذ ExitBootServices وdirect entry |
| Firmware + TPM | OVMF 2024.02، QEMU `q35`، 256MiB، swtpm TPM2 | PASS experimental | `make test-tcg2`؛ markers TCG2 للـ kernel/initrd وPCR4 من Linux sysfs |
| Kernel | Ubuntu 6.8.0-138-generic bzImage/PE-COFF | PASS | real asset inspector، EFI-stub، وdirect boot |
| Initrd | BusyBox static controlled initramfs | PASS | EFI-stub/direct يصلان إلى markers `/init` |
| Initrd | Ubuntu compressed initrd | PASS for tested start | EFI-stub/direct يبدآن Linux ويصلان إلى `/init`; لا يوجد root disk في QEMU، لذلك قد يحدث panic لاحقاً |
| Boot path | Linux PE/COFF EFI stub via UEFI `LoadImage`/`StartImage` | PASS in q35 matrix | no external bootloader |
| Boot path | Linux 64-bit direct boot protocol | PASS for recorded Ubuntu/QEMU matrix | `kernel_address+0x200`، GDT، identity paging، RSI boot_params، direct assembly |
| Boot path | Legacy 16/32-bit direct protocol | Unsupported | لا يوجد real-mode/32-bit backend |
| Firmware ownership | XBoot direct `ExitBootServices` transaction | PASS in direct backend | pre-sized map، allocation freeze، retry helper، QEMU witness |
| Storage | UEFI Simple File System on test ESP | PASS | real kernel/initrd reads |
| Storage | XBoot NVMe/AHCI/USB controller drivers | Unsupported | no controller driver implementation |
| Filesystem | XBoot FAT32 fixture parser | Experimental | host fixtures؛ runtime direct/EFI-stub يستخدم UEFI SFS |
| Security | Secure Boot signature verification | Unsupported | policy boundary only |
| Security | TPM/TCG2 measured boot | Experimental PASS for OVMF+swtpm | `make test-tcg2` يقيس kernel/initrd إلى PCR4 ويقرأ PCR4 غير صفرياً من Linux sysfs؛ لا يثبت TPM عتادياً ولا policy required |
| Other architectures | BIOS، ARM64، RISC-V | Unsupported | no runtime entry evidence |
| Physical hardware | Intel/AMD systems outside QEMU | Unverified | no laboratory or physical-hardware runs |

## Interpretation

تدعم المصفوفة claim ضيقاً وقابلاً لإعادة الإنتاج: XBoot يستطيع البناء كتطبيق UEFI x86_64 مستقل، وتحميل Linux حقيقي، ثم تشغيله إما عبر EFI stub أو عبر direct 64-bit boot protocol في OVMF/QEMU `q35`. لا تدعم المصفوفة certification خارجية، أو universal Linux compatibility، أو توافق كل firmware/storage combinations.
