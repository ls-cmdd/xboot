# XBoot Implementation Plan

## Purpose and execution policy

This document converts `XBoot—MasterBuildPrompt.md` into an auditable engineering plan. Every task has an identifier, a concrete output, and a gate that must pass before the next dependent task is started. A capability is marked **Implemented** only when source code, tests, and an execution witness exist. A capability that has only an interface or directory is marked **Planned** and must not be advertised as supported.

The project follows the required priority order: **correctness**, **reliability**, **security**, then **performance**. Firmware and storage inputs are untrusted. Every allocation, handle, mapping, queue, and parser-owned buffer must have an explicit owner, lifetime, and cleanup path. The project remains independent of GRUB, U-Boot, systemd-boot, Limine, rEFInd, and other bootloader implementations. The supported Linux path uses the EFI stub embedded in the selected kernel through UEFI PE/COFF loading; the direct boot-protocol backend is tracked separately and is not silently substituted.

## Current baseline

The repository now has a freestanding UEFI x86_64 application that validates EFI state, discovers CPUID/ACPI/SMBIOS data, collects a normalized handoff, reads real kernel and initrd files from an ESP through UEFI Simple File System, inspects a Linux 6.8 bzImage, builds a tested protected-code/boot_params plan, and starts the Linux PE/COFF EFI stub. OVMF/QEMU serial evidence reaches Linux kernel output and a separate controlled initramfs reaches `/init` userspace.

| Area | Current state | Next evidence required |
|---|---|---|
| UEFI entry and x86_64 discovery | Implemented for the tested OVMF path | Broader firmware and real-hardware matrix |
| Normalized UEFI handoff | Implemented as pre-StartImage data | Direct XBoot ExitBootServices transaction if a manual backend is pursued |
| Page allocation tracking | Implemented for UEFI backend | Mapping, reclamation and low-memory stress matrix |
| Linux image inspection | Implemented for covered bzImage protocol fields | More kernel versions and malformed real-image corpus |
| Linux PE/COFF EFI-stub path | Implemented for current QEMU path | Firmware matrix, Secure Boot policy, hardware evidence |
| Direct Linux boot protocol handoff | Planned/experimental | 32-bit mode transition, final map transaction, direct entry witness |
| Storage and filesystems | UEFI SFS path implemented; own controller/FAT integration partial | NVMe/AHCI/USB and independent FAT-on-block QEMU tests |
| Hardware graph | Partial read-only PCI/ACPI/SMBIOS coverage | Full graph/resource assignment and failure matrix |
| Security | Fail-closed policy boundary only | Hash/signature/TPM implementations and evidence |
| BIOS, ARM64, RISC-V | Unsupported runtime paths | Separate platform/architecture bring-up evidence |

## Phase and task register

### Phase 0 — Architecture, repository, and build system

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P0-T01 | Freeze module boundaries | `core`, `arch`, `platform`, `mm`, `hal`, `hardware`, `storage`, `fs`, `loader`, `security`, `bootmgr`, `diagnostics`, `recovery`, `ui`, `config`, `logging` boundaries | No platform or filesystem dependency leaks into core interfaces |
| P0-T02 | Define supported configurations | Debug, release, optimized, architecture, and platform selection rules | Unsupported combinations fail explicitly |
| P0-T03 | Define dependency inventory | `docs/DEPENDENCIES.md` with versions, licenses, reasons, and replacement paths | No undocumented external implementation dependency |
| P0-T04 | Make builds reproducible | Deterministic flags, clean build, artifact inspection, and toolchain checks | Two clean builds have equivalent payload hashes where timestamps are normalized |
| P0-T05 | Add release quality gates | Build, unit test, integration test, static checks, documentation, and artifact checks | Release target refuses incomplete gates |

### Phase 1 — UEFI entry and x86_64 initialization

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P1-T01 | Validate EFI image entry | Typed entry ABI and system-table signature validation | OVMF loads the application and emits the entry marker |
| P1-T02 | Capture loaded-image context | Loaded Image Protocol access and device-handle ownership record | Missing protocol is reported as unsupported or an explicit error |
| P1-T03 | Implement watchdog and console policy | Watchdog handling and low-level text/debug output | Debug and release logging policies differ deterministically |
| P1-T04 | Implement x86_64 capability discovery | CPUID vendor, leaves, physical/virtual address widths, NX, SMEP, SMAP, APIC, TSC, virtualization, cache, and topology records | Each capability is detected before use and has a testable status |
| P1-T05 | Add firmware table inventory | ACPI, SMBIOS, framebuffer, and EFI-variable availability records | Missing optional tables do not crash or block policy-independent boot |

### Phase 2 — Memory manager and UEFI handoff

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P2-T01 | Normalize firmware memory descriptors | Versioned XBoot memory-region model with overflow and ordering checks | Malformed descriptors are rejected without out-of-bounds access |
| P2-T02 | Separate temporary and permanent allocation | Boot allocator, temporary allocator, and permanent ownership model | Every allocation has owner and release path |
| P2-T03 | Implement physical page manager | Free/reserved/firmware/kernel/initramfs/framebuffer/ACPI/MMIO region accounting | Allocation never overlaps reserved regions in test fixtures |
| P2-T04 | Implement virtual mapping contract | `xboot_map`/`xboot_unmap`, permissions, identity mapping, NX policy | Invalid alignment, range, and permission combinations fail deterministically |
| P2-T05 | Implement ExitBootServices transaction | Fresh map acquisition, pre-sized map buffer, allocation freeze, map-key retry, and stale-map reporting | `make test-direct-linux` exits boot services before the direct Linux entry |

### Phase 3 — Hardware discovery graph

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P3-T01 | Define hardware graph schema | Stable identifiers, parent/child links, class, vendor, device, capabilities, resources, state, and errors | Graph ownership and destruction are tested |
| P3-T02 | Add CPU and memory nodes | CPU topology and normalized memory nodes | QEMU graph contains CPU and memory with valid relationships |
| P3-T03 | Add ACPI-derived platform nodes | MADT, MCFG, SRAT, SLIT, HPET, and firmware nodes when tables exist | Checksums, lengths, signatures, and bounds are validated |
| P3-T04 | Add PCI root and bridge traversal | Configuration access abstraction, multifunction handling, BAR and capability records | Malformed and inaccessible devices become diagnostic states, not crashes |
| P3-T05 | Add diagnostics export | Text report and machine-readable graph dump | Report includes PASS, WARN, FAIL, UNSUPPORTED, and UNKNOWN reasons |

### Phase 4 — ACPI and SMBIOS parsers

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P4-T01 | Implement RSDP/RSDT/XSDT validation | Independent parser with checksum and length checks | Valid, truncated, checksum-invalid, and overflow fixtures pass expected outcomes |
| P4-T02 | Implement core ACPI tables | MADT, MCFG, FADT, DSDT/SSDT discovery, SRAT, SLIT, HPET records | Each table has version/length bounds and unsupported revision handling |
| P4-T03 | Implement SMBIOS entry discovery | SMBIOS 2.x and 3.x entry validation | Missing and malformed entry points produce diagnostic errors |
| P4-T04 | Parse required SMBIOS structures | System, board, BIOS, processor, memory, manufacturer, product, and firmware metadata | String index and structure-length bounds are tested |
| P4-T05 | Integrate fuzz corpus | Corpus for every parser and deterministic rejection policy | Sanitized host fuzz run finds no memory-safety failure |

### Phase 5 — PCI/PCIe subsystem

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P5-T01 | Define configuration-space HAL | Legacy and extended configuration access with failure status | No direct port/MMIO assumption reaches core |
| P5-T02 | Enumerate buses and bridges | Bus scanning, bridge traversal, multifunction support | QEMU PCI topology is reproduced in the hardware graph |
| P5-T03 | Implement BAR/resource discovery | 32-bit, 64-bit, I/O, MMIO, invalid BAR, and overlap handling | Resource ranges are checked for overflow and conflict |
| P5-T04 | Parse capabilities and interrupt support | MSI, MSI-X, and capability-chain validation | Broken chains terminate safely and become diagnostics |
| P5-T05 | Add PCI failure injection | Inaccessible config, malformed header, invalid BAR, and bridge-loop fixtures | All injected failures are bounded and reported |

### Phase 6 — Storage drivers

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P6-T01 | Define block-device contract | Identify, block size, bounded read, timeout, reset, and ownership APIs | Filesystem code depends only on block contract |
| P6-T02 | Implement minimal NVMe read path | Controller identification, namespace selection, queue setup, logical block size, read, timeout, recovery | QEMU NVMe disk can provide a sector read and failure is reported |
| P6-T03 | Implement AHCI/SATA path | Controller/port discovery, identify, read, timeout, basic reset | QEMU SATA test passes with no NVMe dependency |
| P6-T04 | Define USB host-controller layers | USB core, xHCI priority, device, mass-storage boundaries | Unsupported host controllers are explicit, not silently accepted |
| P6-T05 | Add storage resource tracking | Queues, DMA buffers, MMIO windows, and device handles | Cleanup is verified for success and timeout paths |

### Phase 7 — VFS and filesystems

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P7-T01 | Define VFS interfaces | mount, open, close, read, seek, stat, readdir, exists, load | VFS has no storage-controller knowledge |
| P7-T02 | Implement FAT32 reader | BPB validation, FAT bounds, cluster traversal, directory parsing, file reads | Valid and corrupted FAT32 images pass expected tests |
| P7-T03 | Integrate FAT32 over block devices | Device-independent mount and path loading | QEMU ESP loads a known fixture through VFS |
| P7-T04 | Implement EXT4 read-only path | Superblock, group descriptors, extents/direct blocks as required, directory and file reads | Read-only fixture tests pass; unsupported features are explicit |
| P7-T05 | Define later filesystem boundaries | XFS/Btrfs planned status and no false support claim | Compatibility matrix matches actual tests |

### Phase 8 — Linux loader and initramfs

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P8-T01 | Implement bounded bzImage header parser | `HdrS`, boot protocol, setup size, payload offsets, alignment, relocatability, initrd limits | Valid, truncated, unsupported-version, overflow, and bad-magic fixtures pass |
| P8-T02 | Implement kernel image placement | Protected code plus `init_size` footprint, alignment, initrd limits, and ownership release | Host/QEMU direct gate uses the allocated footprint and rejects bounds failures |
| P8-T03 | Implement command-line builder | Length-limited, NUL-terminated, validated command line placement | Oversize and invalid strings are rejected with precise errors |
| P8-T04 | Implement initramfs loader | Discovery, validation, alignment, placement, reservation, and common format handling | Valid and corrupt initramfs fixtures pass |
| P8-T05 | Construct Linux boot parameters | Zero page, setup header, command line, ramdisk fields, ACPI pointer, and E820 records | Host assertions plus Linux direct QEMU execution |
| P8-T06 | Implement controlled Linux handoff | 64-bit GDT/identity paging, RSI boot_params, non-returning assembly entry, and final firmware transaction | `make test-direct-linux` reaches Linux kernel and controlled `/init`; broader firmware support remains open |

### Phase 9 — Configuration and kernel management

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P9-T01 | Define `xboot.conf` grammar | Versioned grammar, tokenizer, parser, defaults, and error locations | Invalid syntax never causes an uncontrolled boot |
| P9-T02 | Implement boot-entry validation | Kernel, initramfs, architecture, memory, command line, policy, and target checks | Each failure emits a specific diagnostic message |
| P9-T03 | Implement kernel manager | A/B/C/recovery metadata, default selection, version detection, validation, fallback | Failed boot tracking selects recovery without boot loop |
| P9-T04 | Implement text boot manager | Kernel/OS/recovery/diagnostics/firmware setup menu abstraction | Text UI works without a full graphics stack |
| P9-T05 | Implement fast and full paths | Cached selection and minimal probing versus diagnostic full discovery | Both paths have timing markers and equivalent safety checks |

### Phase 10 — Recovery, diagnostics, and boot state

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P10-T01 | Define diagnostic status model | PASS/WARN/FAIL/UNSUPPORTED/UNKNOWN with cause and subsystem | Every missing capability has a reason |
| P10-T02 | Implement diagnostic report | CPU, RAM, firmware, ACPI, PCI, storage, GPU, USB, network, TPM, IOMMU, boot medium, config, kernel, security | Text report can be saved and replayed in tests |
| P10-T03 | Implement recovery manager | Corrupt kernel/initramfs/config/filesystem/security/hardware paths | Recovery decision is deterministic and boot-loop resistant |
| P10-T04 | Implement persistent boot state | Last/successful/failed boot, selected kernel, recovery state, counter | Interrupted writes cannot produce an accepted torn state |
| P10-T05 | Implement developer/release modes | Verbose metadata dumps isolated from release interfaces | Release build disables unsafe debug paths |

### Phase 11 — Security and TPM architecture

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P11-T01 | Define threat model | Kernel, initramfs, filesystem, config, firmware, ACPI, rollback, and tampering threats | Every threat maps to defense or documented limitation |
| P11-T02 | Define security policy boundary | Policy separated from crypto implementation and boot policy | Policy decisions are testable without UI |
| P11-T03 | Implement image hashing | Bounded hashing API and digest metadata | Known-answer tests pass |
| P11-T04 | Implement signature policy boundary | Verification interface, key/config handling, failure states | No Secure Boot claim until the full UEFI chain is implemented |
| P11-T05 | Implement TPM discovery/measurement boundary | TPM version discovery, optional measurement records, PCR/event-log roadmap | Missing TPM follows policy rather than accidental failure |

### Phase 12 — BIOS and architecture portability

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P12-T01 | Define BIOS platform interface | Entry, memory information, disk access, protected/long mode transition contracts | BIOS assumptions do not appear in core headers |
| P12-T02 | Implement BIOS boot sector path | Assembly entry and transition into common handoff | QEMU BIOS boot reaches a diagnostic checkpoint |
| P12-T03 | Define ARM64 architecture contract | Entry, CPU state, page granularity, firmware boundary | Build can select ARM64 and explicitly report unimplemented platform path |
| P12-T04 | Define RISC-V architecture contract | Entry, CPU state, page and firmware boundary | Build can select RISC-V and explicitly report unimplemented platform path |
| P12-T05 | Add portability audit | Search and review for x86_64 assumptions in core/mm/loader APIs | All remaining assumptions are isolated and documented |

### Phase 13 — Testing, fuzzing, failure injection, and performance

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P13-T01 | Build parser unit-test harness | ACPI, SMBIOS, PCI, config, filesystem, kernel, initramfs fixtures | Unit tests cover valid and malformed inputs |
| P13-T02 | Build QEMU matrix | UEFI, BIOS, no disk, corrupt disk/kernel/initramfs/config, low memory, multiple disks/kernels, PCI, NVMe, network, framebuffer | Matrix produces machine-readable pass/fail output |
| P13-T03 | Add failure injection | Timeouts, missing devices, bad maps, corrupt metadata, insufficient memory | No injected case causes uncontrolled crash or false success |
| P13-T04 | Add fuzz targets | Firmware/storage parsers and boot metadata fuzz entry points | Sanitized fuzzing finds no memory-safety defect in the tested corpus |
| P13-T05 | Add benchmarks | Firmware handoff, initialization, discovery, filesystem, kernel/initramfs load, total boot time | Measurements are reproducible and stored with build metadata |
| P13-T06 | Add real-hardware matrix | Intel/AMD, GPU families, NVMe, SATA, USB, firmware variants | Only tested combinations are marked Supported |

### Phase 14 — Documentation, compatibility, and release

| ID | Task | Deliverable | Completion gate |
|---|---|---|---|
| P14-T01 | Complete technical documentation | Architecture, building, development, protocol, memory, hardware, storage, filesystem, loader, security, recovery, diagnostics, testing, porting, troubleshooting | Documentation describes implementation and limitations accurately |
| P14-T02 | Generate compatibility matrix | Architecture, firmware, filesystem, storage, Linux, security, status | Supported/Experimental/Planned/Unsupported labels match evidence |
| P14-T03 | Define release process | Versioning, reproducible artifacts, signing policy, changelog, rollback | Release checklist is executable from a clean checkout |
| P14-T04 | Run final Definition of Done audit | All 23 requirements from MasterBuildPrompt checked individually | Version 1.0 is withheld if any required evidence is missing |
| P14-T05 | Package source and artifacts | Source archive, EFI binary, ESP image, logs, test reports, checksums | Deliverables are readable and reproducible by a second operator |

## Phase completion rule

A phase is complete only when its code builds, unit tests pass, integration tests pass where applicable, QEMU evidence exists where applicable, error cases are tested, memory validation passes, static checks pass, documentation is updated, and performance is measured when the phase affects boot time. If a gate cannot yet be satisfied, the task remains **In Progress** or **Blocked**; it is never silently marked complete.

## References

1. [UEFI Specification 2.10 — EFI System Table](https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html)
2. [UEFI Specification 2.10 — EFI Loaded Image Protocol](https://uefi.org/specs/UEFI/2.10/09_Protocols_EFI_Loaded_Image.html)
3. [Linux/x86 Boot Protocol](https://docs.kernel.org/arch/x86/boot.html)
4. [Linux kernel source tree](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/)
