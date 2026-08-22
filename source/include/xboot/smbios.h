#ifndef XBOOT_SMBIOS_H
#define XBOOT_SMBIOS_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef xboot_status_t (*xboot_smbios_read)(void *context,
                                            uint64_t address,
                                            void *destination,
                                            size_t size);

typedef struct {
    void *context;
    xboot_smbios_read read;
} xboot_smbios_source;

#define XBOOT_SMBIOS_STRING_MAX 64U

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint64_t table_address;
    uint32_t table_length;
    uint16_t structure_count;
    uint16_t type0_count;
    uint16_t type1_count;
    uint16_t type2_count;
    uint16_t type4_count;
    uint16_t type16_count;
    uint16_t type17_count;
    char bios_vendor[XBOOT_SMBIOS_STRING_MAX];
    char bios_version[XBOOT_SMBIOS_STRING_MAX];
    char system_manufacturer[XBOOT_SMBIOS_STRING_MAX];
    char system_product[XBOOT_SMBIOS_STRING_MAX];
    char board_manufacturer[XBOOT_SMBIOS_STRING_MAX];
    char board_product[XBOOT_SMBIOS_STRING_MAX];
    char processor_version[XBOOT_SMBIOS_STRING_MAX];
} xboot_smbios_summary;

xboot_status_t xboot_smbios_parse(const xboot_smbios_source *source,
                                  uint64_t entry_address,
                                  xboot_smbios_summary *out_summary);

#endif
