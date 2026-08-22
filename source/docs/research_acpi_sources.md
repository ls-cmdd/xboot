# ACPI parser research notes

## Sources reviewed

1. The UEFI Forum ACPI 6.5 documentation at https://uefi.org/specs/ACPI/6.5/ defines the system description table architecture and includes RSDP, RSDT, XSDT, FADT, MADT, SRAT, SLIT, HPET, and related table definitions.

2. The ACPI software programming model at https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html states that all system description tables begin with a common header, that the XSDT references other tables, and that ACPI table values are little-endian. On UEFI systems the RSDP pointer is obtained from EFI configuration-table GUID entries; ACPI 2.0+ and ACPI 1.0 GUIDs are distinct.

## Implementation implications

XBoot's parser must validate the RSDP signature, revision-dependent length, checksums, and the physical addresses of RSDT/XSDT before walking entries. Every child table must validate its common signature, declared length, and checksum before any table-specific field is read. Entry-size arithmetic must be overflow-safe, and a malformed table must become a diagnostic error rather than a crash. The first implementation will discover and count RSDP/XSDT/RSDT/MADT/MCFG/FADT/HPET/SRAT/SLIT records without interpreting AML or executing control methods.
