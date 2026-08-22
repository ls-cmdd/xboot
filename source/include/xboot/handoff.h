#ifndef XBOOT_HANDOFF_H
#define XBOOT_HANDOFF_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/uefi.h>

typedef struct {
    uint32_t type;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t page_count;
    uint64_t attribute;
} xboot_memory_region;

typedef struct {
    xboot_memory_region *memory_regions;
    size_t memory_region_count;
    size_t memory_descriptor_size;
    uint32_t memory_descriptor_version;
    UINTN uefi_map_key;
    void *acpi_table;
    void *smbios_table;
    uint32_t firmware_revision;
    uint32_t system_table_revision;
    void *uefi_memory_map;
    UINTN uefi_memory_map_capacity;
} xboot_uefi_handoff;

xboot_status_t xboot_uefi_collect_handoff(EFI_SYSTEM_TABLE *system_table,
                                           EFI_BOOT_SERVICES *boot_services,
                                           xboot_uefi_handoff **out_handoff);
void xboot_uefi_release_handoff(EFI_BOOT_SERVICES *boot_services,
                                xboot_uefi_handoff *handoff);
xboot_status_t xboot_uefi_expand_memory_map(EFI_BOOT_SERVICES *boot_services,
                                             xboot_uefi_handoff *handoff,
                                             UINTN additional_descriptors);

#endif
