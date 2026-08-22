#include <xboot/acpi.h>

#define XBOOT_ACPI_HEADER_SIZE 36U
#define XBOOT_ACPI_RSDP_V1_SIZE 20U
#define XBOOT_ACPI_MAX_TABLE_LENGTH (1024U * 1024U)
#define XBOOT_ACPI_MAX_TABLES 4096U

static int bytes_equal(const uint8_t *a, const char *b, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (a[i] != (uint8_t)b[i]) {
            return 0;
        }
    }
    return 1;
}

static uint16_t read_u16(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint64_t read_u64(const uint8_t *bytes) {
    return (uint64_t)read_u32(bytes) | ((uint64_t)read_u32(bytes + 4) << 32);
}

static uint8_t checksum_buffer(const uint8_t *bytes, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum = (uint8_t)(sum + bytes[i]);
    }
    return sum;
}

static xboot_status_t read_exact(const xboot_acpi_source *source,
                                 uint64_t address,
                                 void *destination,
                                 size_t size) {
    if (source == NULL || source->read == NULL || destination == NULL || size == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    return source->read(source->context, address, destination, size);
}

static xboot_status_t validate_sdt(const xboot_acpi_source *source,
                                   uint64_t address,
                                   uint8_t header[XBOOT_ACPI_HEADER_SIZE],
                                   uint32_t *out_length) {
    uint32_t length;
    uint8_t *table;
    xboot_status_t status;
    if (address == 0 || header == NULL || out_length == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = read_exact(source, address, header, XBOOT_ACPI_HEADER_SIZE);
    if (status != XBOOT_OK) {
        return status;
    }
    length = read_u32(header + 4);
    if (length < XBOOT_ACPI_HEADER_SIZE || length > XBOOT_ACPI_MAX_TABLE_LENGTH) {
        return XBOOT_CORRUPT;
    }
    table = (uint8_t *)source->context;
    (void)table;
    *out_length = length;
    return XBOOT_OK;
}

static xboot_status_t checksum_sdt(const xboot_acpi_source *source,
                                    uint64_t address,
                                    uint32_t length) {
    uint8_t buffer[256];
    uint32_t offset = 0;
    uint8_t sum = 0;
    xboot_status_t status;
    while (offset < length) {
        size_t chunk = length - offset;
        if (chunk > sizeof(buffer)) {
            chunk = sizeof(buffer);
        }
        status = read_exact(source, address + offset, buffer, chunk);
        if (status != XBOOT_OK) {
            return status;
        }
        for (size_t i = 0; i < chunk; i++) {
            sum = (uint8_t)(sum + buffer[i]);
        }
        offset += (uint32_t)chunk;
    }
    return sum == 0 ? XBOOT_OK : XBOOT_CORRUPT;
}

static xboot_status_t parse_madt(const xboot_acpi_source *source,
                                  uint64_t address, uint32_t length,
                                  xboot_acpi_summary *summary) {
    uint32_t offset = 44;
    if (length < 44) return XBOOT_CORRUPT;
    while (offset < length) {
        uint8_t entry[2];
        xboot_status_t status = read_exact(source, address + offset, entry, sizeof(entry));
        if (status != XBOOT_OK) return status;
        if (entry[1] < 2 || entry[1] > length - offset) return XBOOT_CORRUPT;
        if (entry[0] == 0 && entry[1] >= 8) summary->madt_local_apic_count++;
        if (entry[0] == 1 && entry[1] >= 12) summary->madt_io_apic_count++;
        if (entry[0] == 9 && entry[1] >= 16) summary->madt_x2apic_count++;
        offset += entry[1];
    }
    return XBOOT_OK;
}

static xboot_status_t parse_mcfg(const xboot_acpi_source *source,
                                  uint64_t address, uint32_t length,
                                  xboot_acpi_summary *summary) {
    uint8_t allocation[16];
    xboot_status_t status;
    if (length < 44 || ((length - 44) % 16) != 0) return XBOOT_CORRUPT;
    summary->mcfg_segment_count = (length - 44) / 16;
    if (summary->mcfg_segment_count == 0) return XBOOT_OK;
    status = read_exact(source, address + 44, allocation, sizeof(allocation));
    if (status != XBOOT_OK) return status;
    summary->mcfg_base_address = read_u64(allocation);
    summary->mcfg_segment_group = read_u16(allocation + 8);
    summary->mcfg_start_bus = allocation[10];
    summary->mcfg_end_bus = allocation[11];
    if (summary->mcfg_base_address == 0 ||
        (summary->mcfg_base_address & 0xfffffULL) != 0 ||
        summary->mcfg_start_bus > summary->mcfg_end_bus) {
        return XBOOT_CORRUPT;
    }
    return XBOOT_OK;
}

static xboot_status_t parse_fadt(const xboot_acpi_source *source,
                                  uint64_t address, uint32_t length,
                                  xboot_acpi_summary *summary) {
    uint8_t bytes[8];
    xboot_status_t status;
    if (length >= 44) {
        status = read_exact(source, address + 40, bytes, 4);
        if (status != XBOOT_OK) return status;
        summary->fadt_dsdt_address = read_u32(bytes);
    }
    if (length >= 148) {
        status = read_exact(source, address + 140, bytes, 8);
        if (status != XBOOT_OK) return status;
        if (read_u64(bytes) != 0) summary->fadt_dsdt_address = read_u64(bytes);
    }
    return XBOOT_OK;
}

static void count_table(xboot_acpi_summary *summary, const uint8_t header[36]) {
    summary->table_count++;
    if (bytes_equal(header, "APIC", 4)) summary->madt_count++;
    if (bytes_equal(header, "MCFG", 4)) summary->mcfg_count++;
    if (bytes_equal(header, "FACP", 4)) summary->fadt_count++;
    if (bytes_equal(header, "HPET", 4)) summary->hpet_count++;
    if (bytes_equal(header, "SRAT", 4)) summary->srat_count++;
    if (bytes_equal(header, "SLIT", 4)) summary->slit_count++;
    if (bytes_equal(header, "DSDT", 4)) summary->dsdt_count++;
    if (bytes_equal(header, "SSDT", 4)) summary->ssdt_count++;
}

xboot_status_t xboot_acpi_parse(const xboot_acpi_source *source,
                                uint64_t rsdp_address,
                                xboot_acpi_summary *out_summary) {
    uint8_t rsdp[36];
    uint8_t root_header[36];
    uint8_t child_header[36];
    uint64_t root_address;
    uint32_t root_length;
    uint32_t entry_size;
    uint32_t entry_count;
    xboot_status_t status;

    if (source == NULL || source->read == NULL || out_summary == NULL ||
        rsdp_address == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < sizeof(*out_summary); i++) {
        ((uint8_t *)out_summary)[i] = 0;
    }
    status = read_exact(source, rsdp_address, rsdp, XBOOT_ACPI_RSDP_V1_SIZE);
    if (status != XBOOT_OK) {
        return status;
    }
    if (!bytes_equal(rsdp, "RSD PTR ", 8) || checksum_buffer(rsdp, 20) != 0) {
        return XBOOT_CORRUPT;
    }
    out_summary->revision = rsdp[15];
    if (out_summary->revision >= 2) {
        status = read_exact(source, rsdp_address, rsdp, sizeof(rsdp));
        if (status != XBOOT_OK) {
            return status;
        }
        uint32_t rsdp_length = read_u32(rsdp + 20);
        if (rsdp_length < sizeof(rsdp) || rsdp_length > XBOOT_ACPI_MAX_TABLE_LENGTH) {
            return XBOOT_CORRUPT;
        }
        status = checksum_sdt(source, rsdp_address, rsdp_length);
        if (status != XBOOT_OK) {
            return status;
        }
        root_address = read_u64(rsdp + 24);
    } else {
        root_address = read_u32(rsdp + 16);
    }
    if (root_address == 0) {
        return XBOOT_CORRUPT;
    }
    status = validate_sdt(source, root_address, root_header, &root_length);
    if (status != XBOOT_OK) {
        return status;
    }
    if (out_summary->revision >= 2 && bytes_equal(root_header, "XSDT", 4)) {
        entry_size = 8;
    } else if (bytes_equal(root_header, "RSDT", 4)) {
        entry_size = 4;
    } else {
        return XBOOT_PROTOCOL_ERROR;
    }
    status = checksum_sdt(source, root_address, root_length);
    if (status != XBOOT_OK) {
        return status;
    }
    if ((root_length - XBOOT_ACPI_HEADER_SIZE) % entry_size != 0) {
        return XBOOT_CORRUPT;
    }
    entry_count = (root_length - XBOOT_ACPI_HEADER_SIZE) / entry_size;
    if (entry_count == 0 || entry_count > XBOOT_ACPI_MAX_TABLES) {
        return XBOOT_CORRUPT;
    }
    out_summary->root_table_address = root_address;
    out_summary->root_table_length = root_length;
    for (uint32_t i = 0; i < entry_count; i++) {
        uint8_t entry_bytes[8];
        uint64_t table_address;
        uint32_t table_length;
        status = read_exact(source,
                            root_address + XBOOT_ACPI_HEADER_SIZE + (uint64_t)i * entry_size,
                            entry_bytes, entry_size);
        if (status != XBOOT_OK) {
            return status;
        }
        table_address = entry_size == 8 ? read_u64(entry_bytes) : read_u32(entry_bytes);
        status = validate_sdt(source, table_address, child_header, &table_length);
        if (status != XBOOT_OK) {
            return status;
        }
        status = checksum_sdt(source, table_address, table_length);
        if (status != XBOOT_OK) {
            return status;
        }
        count_table(out_summary, child_header);
        if (bytes_equal(child_header, "APIC", 4)) {
            status = parse_madt(source, table_address, table_length, out_summary);
        } else if (bytes_equal(child_header, "MCFG", 4)) {
            status = parse_mcfg(source, table_address, table_length, out_summary);
        } else if (bytes_equal(child_header, "FACP", 4)) {
            status = parse_fadt(source, table_address, table_length, out_summary);
        }
        if (status != XBOOT_OK) return status;
    }
    return XBOOT_OK;
}
