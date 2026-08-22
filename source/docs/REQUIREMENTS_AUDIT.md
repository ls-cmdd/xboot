# MasterBuildPrompt Requirements Audit

## Audit scope

يعامل هذا التدقيق المصدر والاختبارات وسجل QEMU والتوثيق كفئات أدلة منفصلة. لا تُصنف capability كـ **Implemented** إلا إذا كان مسارها موجوداً، وسلوك فشلها معرفاً، ويوجد اختبار أو execution witness مناسب. وجود directory أو API وحده لا يكفي.

## Summary

| MasterBuildPrompt domain | Current status | Evidence or limitation |
|---|---|---|
| Independent repository and build system | Implemented | Clang/LLD freestanding PE/COFF build، FAT32 ESP image، لا backend لbootloader خارجي |
| UEFI application entry | Implemented | `platform/uefi/main.c` وOVMF/QEMU |
| x86_64 CPUID discovery | Implemented for current records | vendor/features/address widths/topology؛ المصفوفة الأوسع ما زالت مطلوبة |
| EFI system table/watchdog | Implemented | signature validation وwatchdog disable |
| ACPI/SMBIOS discovery and bounded parsing | Implemented for covered tables | checksums/bounds، malformed fixtures، QEMU witnesses |
| UEFI memory map and normalized handoff | Implemented | retained map، capacity expansion، direct final transaction |
| Page allocation ownership | Implemented for tested UEFI backend | host/QEMU lifecycle؛ advanced reclamation policy ما زالت محدودة |
| Block I/O and GPT | Implemented bounded read-only components | host tests وUEFI Block I/O witness؛ raw FAT ESP ليس GPT |
| UEFI filesystem loading | Implemented for tested Simple File System path | kernel/initrd/config reads from real ESP؛ FAT32 core يملك real-ESP 8.3 read-only integration evidence |
| Linux bzImage inspection | Implemented | HdrS، protocol 2.15، setup/syssize، protected/payload bounds، real asset |
| Linux placement/boot_params/E820 | Implemented for tested direct path | `max(protected_size, init_size)`، E820 coalescing، host assertions، QEMU direct witness، و`AllocateMaxAddress` تحت 4GiB |
| Linux PE/COFF EFI-stub start | Implemented for QEMU/OVMF matrix | Device Path، LoadImage/StartImage، LoadOptions، serial witness |
| Linux userspace | Implemented for controlled QEMU artifacts | EFI-stub/direct reach `/init` markers |
| Linux direct boot-protocol handoff | Implemented for recorded Ubuntu/QEMU matrix | GDT/identity map/RSI/assembly/ExitBootServices؛ broader kernels remain unverified |
| initramfs | Implemented in tested layers | newc/crc fixtures؛ compressed Ubuntu initrd is loaded and delegated to Linux |
| Configuration/kernel manager/UI | Partial | `xboot.conf` is parsed and used for paths/options; no interactive menu or multi-entry selection yet |
| PCI/PCIe controller/resource subsystem | Experimental/partial | generic read-only enumerator only؛ no BAR assignment |
| NVMe/AHCI/USB | Planned | no production controller drivers؛ runtime يعتمد UEFI SFS |
| Artifact integrity | Implemented allowlist | freestanding SHA-256، hash-locked config، QEMU tamper rejection; this is not Secure Boot |
| Security/Secure Boot | Experimental/implemented as firmware delegation | قراءة `SecureBoot`، `secure_boot=required`، و`LoadImage`/`UnloadImage` authentication قبل direct handoff؛ لا إدارة PK/KEK/db/dbx أو verifier Authenticode مستقل |
| TPM/TCG2 measured boot | Experimental/implemented for tested QEMU path | TCG2 adapter، optional و`measure=required` policies، host ABI tests، و`make test-tcg2` يثبت positive PCR4 وnegative no-TPM rejection؛ TPM العتادي غير مثبت |
| BIOS/ARM64/RISC-V | Unsupported | no runtime bring-up |
| QEMU matrix | Partial but materially extended | OVMF x86_64 q35، EFI-stub/direct، 256/512MiB، TCG2+swtpm positive/required-negative، Secure Boot SnakeOil signed/unsigned، positive/negative hash gates |
| Fuzzing/static analysis/performance | Partial but extended | strict compiler، host gates، marker/symbol checks، وASan/UBSan parser fuzz-smoke على 100000 input؛ full corpus/performance matrix remains |
| Real hardware/external certification | Unsupported/unverified | no physical hardware or independent laboratory evidence |

## Current compliance decision

XBoot now has two real end-to-end Linux paths. The default PE/COFF path starts the Linux EFI stub through UEFI. The `DIRECT=1` path performs XBoot-owned placement, boot_params construction, final `ExitBootServices`, and direct 64-bit Linux entry. Neither path chainloads another bootloader. The direct path is proven by `make test-direct-linux` with Ubuntu 6.8.0-138-generic in OVMF/QEMU `q35`.

The repository does not claim universal Linux compatibility. The execution witness is limited to the recorded Ubuntu kernel/initrds, OVMF/QEMU `q35`, and the documented memory sizes. Other kernels, firmware implementations, initrd formats, controllers, GPUs, and physical machines require separate evidence.

The SHA-256 allowlist is an artifact-integrity feature. It rejects the intentionally tampered QEMU kernel and can be used to pin known artifacts, but it is not UEFI Secure Boot: XBoot does not yet parse or enforce PK/KEK/db/dbx or PE Authenticode certificate chains.

## Release-gate policy

A release candidate must pass `make clean test`, `make test-linux-userspace`, `make test-tcg2`, `make test-secureboot`, `make test-direct-linux`, `make test-hash-negative`, `make test-real-fat32`, `make fuzz-smoke`, and `tools/quality_gate.sh`; retain QEMU serial/debug logs, TCG2/PCR markers, Secure Boot signed/unsigned logs, real-ESP parser result, fuzz sanitizer result, and artifact hashes; and keep the compatibility matrix consistent with the evidence. External certification remains a separate activity requiring a defined standard and independent hardware/laboratory test records.

## References

1. [The EFI Boot Stub — Linux Kernel Documentation](https://docs.kernel.org/admin-guide/efi-stub.html)
2. [The Linux/x86 Boot Protocol](https://docs.kernel.org/arch/x86/boot.html)
3. [UEFI Secure Boot and Driver Signing](https://uefi.org/specs/UEFI/2.10/32_Secure_Boot_and_Driver_Signing.html)
4. [UEFI Loaded Image Protocol](https://uefi.org/specs/UEFI/2.10/09_Protocols_EFI_Loaded_Image.html)
