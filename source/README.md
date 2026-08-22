# XBoot

XBoot is an independent bootloader project for the Xenon system. It is being developed from the smallest honest boot path outward: the current milestone is a freestanding **UEFI x86_64 application** that validates the EFI system table, disables the firmware watchdog, discovers CPU capabilities, locates ACPI and SMBIOS configuration tables, and reads the UEFI memory map.

> The repository deliberately documents unsupported capabilities instead of using fake drivers, placeholder parsers, or simulated boot success.

## Current build

The initial build uses Clang/LLVM to emit a freestanding PE/COFF UEFI application and `lld-link` to link it with no host runtime. QEMU and OVMF provide the reproducible firmware test target. The project does not use GRUB, U-Boot, systemd-boot, Limine, rEFInd, or another bootloader backend.

```sh
make all       # build build/xboot.efi
make check     # inspect PE/COFF headers and sections
make image     # create build/xboot-esp.img
make test      # boot the image with QEMU + OVMF and assert discovery logs
make run       # run interactively through the debug console
make clean
```

## Design boundaries

The core is being kept independent from firmware and architecture code. The UEFI ABI definitions in `include/xboot/uefi.h` are the minimum definitions needed for the current phase; they are not presented as a complete UEFI SDK. The future Linux loader must follow the Linux x86 boot protocol and must validate kernel headers before any handoff.

## Repository layout

The directory layout reserves separate boundaries for `core`, `mm`, `hal`, `hardware`, `storage`, `fs`, `loader`, `security`, `bootmgr`, `diagnostics`, `recovery`, `ui`, `platform`, and `arch`. Empty directories are architectural boundaries, not claims that those subsystems already work.

## References

1. [UEFI Specification 2.10 — EFI System Table](https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html)
2. [UEFI Specification 2.10 — EFI Loaded Image Protocol](https://uefi.org/specs/UEFI/2.10/09_Protocols_EFI_Loaded_Image.html)
3. [Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/v6.1/x86/boot.html)
