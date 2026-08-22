# XBoot implementation status

ينفذ المستودع حالياً تطبيق UEFI مستقل لـ x86_64 مع مسارين Linux فعليين. المسار الافتراضي يستخدم PE/COFF EFI stub عبر UEFI `LoadImage`/`StartImage`. والمسار `DIRECT=1` يستخدم Linux 64-bit boot protocol مباشرةً: يبني boot_params/E820، يخصص kernel footprint وstack/page tables، يبني identity map لأول 4GiB، ينفذ final `ExitBootServices` transaction، ثم ينتقل عبر assembly إلى kernel entry `kernel_address + 0x200`. لا يعتمد أي من المسارين على GRUB أو U-Boot أو Limine أو systemd-boot أو rEFInd.

| Capability | Status | Evidence |
|---|---|---|
| Standalone UEFI application entry | Implemented | `platform/uefi/main.c` وPE/COFF link |
| x86_64 CPU discovery | Implemented | CPUID records، host/QEMU |
| EFI system-table validation and watchdog | Implemented | signature check وwatchdog disable |
| ACPI/SMBIOS discovery and bounded parsing | Implemented for covered tables | checksums/bounds، malformed fixtures، QEMU |
| Read-only PCI configuration enumerator | Experimental | host fixture؛ لا يوجد resource assignment |
| Block abstraction and UEFI Block I/O | Implemented/Experimental | checked reads، adapter tests، QEMU witness |
| GPT parser | Implemented bounded parser | CRC/range/overlap tests؛ raw ESP ليس GPT |
| FAT32 core parser | Implemented for bounded 8.3 read-only path | fixture-tested وreal 128MiB ESP integration؛ runtime الرئيسي ما زال UEFI SFS |
| UEFI Simple File System adapter | Implemented | fake protocol tests وreal ESP reads |
| Page-backed UEFI file loader | Implemented | host test وQEMU integration |
| Linux bzImage inspector | Implemented | HdrS/protocol 2.15/setup/syssize/payload/handover وreal asset |
| Linux placement footprint | Implemented for tested direct path | يحجز `max(protected_size, init_size)` ويستخدم direct backend مقيّداً بـ `AllocateMaxAddress` تحت 4GiB |
| Linux boot_params/E820 | Implemented for tested direct path | host assertions وQEMU direct execution |
| Linux PE/COFF EFI-stub start | Implemented | Device Path، LoadImage/StartImage، LoadOptions، QEMU |
| Linux direct 64-bit boot protocol | Implemented for current QEMU matrix | `loaders/linux/direct.c` و`arch/x86_64/linux_entry.S`، target `make test-direct-linux` |
| Final XBoot ExitBootServices transaction | Implemented for direct path | pre-sized map buffer، allocation freeze، retry helper، QEMU direct witness |
| Linux kernel start in QEMU | Implemented for tested Ubuntu asset | EFI-stub and direct paths print `Linux version` |
| Linux userspace proof in QEMU | Implemented for controlled initramfs | direct path reaches `/init` markers at 512MiB؛ EFI-stub path also tested |
| Initramfs newc/crc inspector | Implemented | bounded fixture tests؛ compressed initrd delegated to kernel path |
| Boot configuration parser/manager | Experimental | deterministic host tests؛ runtime paths remain interim fixed paths |
| Recovery diagnostics/security policy boundary | Implemented boundary | host tests؛ no PK/KEK/db or TPM claim |
| SHA-256 artifact integrity allowlist | Implemented | freestanding vectors، hash-locked config، fail-closed runtime mismatch path |
| Normalized UEFI handoff/page ownership | Implemented | map collection، resource/page lifecycle، direct transaction |
| Automated quality gate | Implemented | host، real FAT32 ESP، parser fuzz-smoke، EFI-stub، direct، userspace، TCG2 positive/required-negative، Secure Boot signed/unsigned، hash-negative، marker/unresolved-symbol checks؛ يعيد default ESP بعد الاختبار |
| Secure Boot enforcement | Experimental/implemented for tested firmware-delegated path | يقرأ `SecureBoot` ويفوض authentication إلى UEFI `LoadImage`/`UnloadImage` قبل direct handoff؛ لا يوجد verifier مستقل أو إدارة PK/KEK/db/dbx |
| TPM/TCG2 measurement backend | Experimental/implemented for tested QEMU path | `LocateProtocol`/`GetCapability`/`HashLogExtendEvent`، PCR4، optional و`measure=required` policies، host ABI fixtures، و`make test-tcg2` يثبت positive/negative behaviour عبر OVMF+swtpm |
| NVMe/AHCI/USB drivers | Not implemented | no controller-driver claim |
| BIOS, ARM64, RISC-V runtime ports | Not implemented | architecture boundary only |
| Real hardware support and external certification | Unsupported/unverified | no physical hardware or independent laboratory evidence |

## Evidence boundary

يثبت `make test-direct-linux` أن XBoot يستطيع، في OVMF/QEMU `q35` وبـ512MiB، التحقق من SHA-256 للـ artifacts، تخصيص الذاكرة تحت 4GiB عبر `AllocateMaxAddress`، تحميل kernel Ubuntu 6.8.0-138-generic وinitramfs مصغراً، الخروج من boot services، الوصول إلى Linux 64-bit entry، ثم تنفيذ `/init`. ويثبت جزءه الثاني بدء Linux مع Ubuntu initrd الحقيقي حتى `Run /init as init process`. ويثبت `make test-tcg2`، مع OVMF وTPM2 emulator من `swtpm`، أن XBoot يستدعي TCG2 فعلياً لقياس kernel وinitrd إلى PCR4، وأن Linux يرى PCR4 غير صفرياً من sysfs، وأن `measure=required` يمنع boot عند غياب TPM. ويثبت `make test-secureboot` delegated Secure Boot positive/negative path باستخدام OVMF SnakeOil keys. هذا لا يثبت توافق كل kernels أو firmware أو storage devices، ولا يثبت TPM عتادياً أو trust chain إنتاجية.

مسار EFI-stub يبقى backend منفصلاً للرجوع والتوافق، ولا يُستخدم لإخفاء فشل direct backend. TCG2 يملك الآن optional و`measure=required` policies مثبتة في OVMF+swtpm. Secure Boot enforcement delegated إلى firmware ومثبت بمفتاح OVMF SnakeOil، لكنه ليس verifier مستقلاً ولا يثبت production PK/KEK/db/dbx. FAT32 parser يدعم المسار read-only bounded بأسماء 8.3 فقط في evidence الحالي؛ LFN وكتابة FAT وcontroller drivers ومصفوفة hardware الحقيقية والاعتماد الخارجي غير مثبتة، لذلك لا يُعلن XBoot certified أو universal.
