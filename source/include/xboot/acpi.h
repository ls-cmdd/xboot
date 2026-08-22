#ifndef XBOOT_ACPI_H
#define XBOOT_ACPI_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef xboot_status_t (*xboot_acpi_read)(void *context,
                                          uint64_t address,
                                          void *destination,
                                          size_t size);

typedef struct {
    void *context;
    xboot_acpi_read read;
} xboot_acpi_source;

typedef struct {
    uint8_t revision;
    uint64_t root_table_address;
    uint32_t root_table_length;
    uint32_t table_count;
    uint32_t madt_count;
    uint32_t mcfg_count;
    uint32_t fadt_count;
    uint32_t hpet_count;
    uint32_t srat_count;
    uint32_t slit_count;
    uint32_t dsdt_count;
    uint32_t ssdt_count;
    uint32_t madt_local_apic_count;
    uint32_t madt_io_apic_count;
    uint32_t madt_x2apic_count;
    uint32_t mcfg_segment_count;
    uint64_t mcfg_base_address;
    uint16_t mcfg_segment_group;
    uint8_t mcfg_start_bus;
    uint8_t mcfg_end_bus;
    uint64_t fadt_dsdt_address;
} xboot_acpi_summary;

xboot_status_t xboot_acpi_parse(const xboot_acpi_source *source,
                                uint64_t rsdp_address,
                                xboot_acpi_summary *out_summary);

#endif
