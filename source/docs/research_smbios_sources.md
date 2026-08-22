# SMBIOS parser research notes

## Sources reviewed

1. The DMTF SMBIOS Reference Specification 3.8 working draft is available at https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.8.0WIP50.pdf. It defines both the 32-bit SMBIOS 2.1 entry point and the 64-bit SMBIOS 3.0 entry point, common structure headers, string sections, and required structures such as BIOS, system, baseboard, processor, physical memory array, and memory device.

2. DMTF's SMBIOS 3.8 release notice is at https://www.dmtf.org/content/dmtf-releases-smbios-38. SMBIOS is designed for OS-present, OS-absent, and pre-OS environments and provides standardized firmware management information without relying on error-prone hardware probing.

## Implementation implications

The first XBoot parser will validate the `_SM3_` or `_SM_` entry anchor, entry length, checksums, table address, table length, and structure count/termination constraints. Each structure will validate its formatted length and scan the double-NUL-terminated string area within the global table bound. The parser will count and extract bounded metadata for types 0, 1, 2, 4, 16, and 17; unknown types remain skippable only when their bounds are valid.
