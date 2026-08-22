#include <xboot/smbios.h>

#define XBOOT_SMBIOS_ENTRY_MIN 0x18U
#define XBOOT_SMBIOS_MAX_TABLE_LENGTH (1024U * 1024U)
#define XBOOT_SMBIOS_MAX_STRUCTURES 4096U

static uint16_t read_u16(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint64_t read_u64(const uint8_t *bytes) {
    return (uint64_t)read_u32(bytes) | ((uint64_t)read_u32(bytes + 4) << 32);
}

static int bytes_equal(const uint8_t *a, const char *b, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (a[i] != (uint8_t)b[i]) return 0;
    }
    return 1;
}

static uint8_t checksum(const uint8_t *bytes, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) sum = (uint8_t)(sum + bytes[i]);
    return sum;
}

static xboot_status_t read_exact(const xboot_smbios_source *source,
                                 uint64_t address, void *destination, size_t size) {
    if (source == NULL || source->read == NULL || destination == NULL || size == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    return source->read(source->context, address, destination, size);
}

static void copy_string(const uint8_t *table, size_t table_size,
                        uint8_t string_index, char destination[XBOOT_SMBIOS_STRING_MAX]) {
    size_t offset = 0;
    uint8_t current = 1;
    destination[0] = '\0';
    if (string_index == 0) return;
    while (offset < table_size) {
        size_t start = offset;
        while (offset < table_size && table[offset] != 0) offset++;
        if (offset >= table_size) return;
        if (current == string_index) {
            size_t length = offset - start;
            if (length >= XBOOT_SMBIOS_STRING_MAX) length = XBOOT_SMBIOS_STRING_MAX - 1;
            for (size_t i = 0; i < length; i++) destination[i] = (char)table[start + i];
            destination[length] = '\0';
            return;
        }
        current++;
        offset++;
        if (offset < table_size && table[offset] == 0) return;
    }
}

static xboot_status_t read_structure(const xboot_smbios_source *source,
                                     uint64_t address, uint32_t table_end,
                                     uint8_t *buffer, size_t buffer_size,
                                     uint8_t *out_type, uint8_t *out_length,
                                     uint32_t *out_total_size) {
    uint8_t header[4];
    uint8_t byte;
    uint32_t offset;
    xboot_status_t status;
    if (address > UINT64_MAX - table_end || buffer == NULL || buffer_size < 4 ||
        out_type == NULL || out_length == NULL || out_total_size == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = read_exact(source, address, header, sizeof(header));
    if (status != XBOOT_OK) return status;
    if (header[1] < 4) return XBOOT_CORRUPT;
    *out_type = header[0];
    *out_length = header[1];
    if (*out_length > buffer_size) return XBOOT_CORRUPT;
    if (*out_length > table_end) return XBOOT_CORRUPT;
    status = read_exact(source, address, buffer, *out_length);
    if (status != XBOOT_OK) return status;
    offset = *out_length;
    for (;;) {
        if (offset >= table_end) return XBOOT_CORRUPT;
        status = read_exact(source, address + offset, &byte, 1);
        if (status != XBOOT_OK) return status;
        offset++;
        if (byte == 0) {
            if (offset >= table_end) return XBOOT_CORRUPT;
            status = read_exact(source, address + offset, &byte, 1);
            if (status != XBOOT_OK) return status;
            if (byte == 0) {
                offset++;
                if (offset > buffer_size) return XBOOT_CORRUPT;
                status = read_exact(source, address, buffer, offset);
                if (status != XBOOT_OK) return status;
                *out_total_size = offset;
                return XBOOT_OK;
            }
        }
    }
}

xboot_status_t xboot_smbios_parse(const xboot_smbios_source *source,
                                  uint64_t entry_address,
                                  xboot_smbios_summary *out_summary) {
    uint8_t entry[0x20];
    uint8_t structure[256];
    uint8_t type;
    uint8_t formatted_length;
    uint32_t total_size;
    uint64_t table_address;
    uint32_t table_length;
    uint16_t declared_count = 0;
    uint8_t is_v3;
    xboot_status_t status;

    if (source == NULL || source->read == NULL || out_summary == NULL || entry_address == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < sizeof(*out_summary); i++) ((uint8_t *)out_summary)[i] = 0;
    status = read_exact(source, entry_address, entry, sizeof(entry));
    if (status != XBOOT_OK) return status;
    if (bytes_equal(entry, "_SM3_", 5)) {
        uint8_t length = entry[6];
        if (length < 0x18 || length > sizeof(entry) || checksum(entry, length) != 0) {
            return XBOOT_CORRUPT;
        }
        out_summary->major = entry[7];
        out_summary->minor = entry[8];
        table_length = read_u32(entry + 12);
        table_address = read_u64(entry + 16);
        is_v3 = 1;
    } else if (bytes_equal(entry, "_SM_", 4)) {
        uint8_t length = entry[5];
        if (length < 0x1f || length > sizeof(entry) || checksum(entry, length) != 0 ||
            !bytes_equal(entry + 16, "_DMI_", 5) || checksum(entry + 16, 0x0f) != 0) {
            return XBOOT_CORRUPT;
        }
        out_summary->major = entry[6];
        out_summary->minor = entry[7];
        declared_count = read_u16(entry + 0x1c);
        table_length = read_u16(entry + 0x16);
        table_address = read_u32(entry + 0x18);
        is_v3 = 0;
    } else {
        return XBOOT_PROTOCOL_ERROR;
    }
    if (table_address == 0 || table_length == 0 || table_length > XBOOT_SMBIOS_MAX_TABLE_LENGTH) {
        return XBOOT_CORRUPT;
    }
    out_summary->table_address = table_address;
    out_summary->table_length = table_length;
    uint32_t offset = 0;
    while (offset < table_length) {
        uint32_t remaining = table_length - offset;
        status = read_structure(source, table_address + offset, remaining,
                                structure, sizeof(structure), &type,
                                &formatted_length, &total_size);
        if (status != XBOOT_OK) return status;
        if (out_summary->structure_count >= XBOOT_SMBIOS_MAX_STRUCTURES) return XBOOT_CORRUPT;
        out_summary->structure_count++;
        if (type == 127) {
            if (formatted_length < 4) return XBOOT_CORRUPT;
            if (!is_v3 && declared_count != 0 && out_summary->structure_count != declared_count) {
                return XBOOT_CORRUPT;
            }
            return XBOOT_OK;
        }
        switch (type) {
        case 0:
            out_summary->type0_count++;
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[4], out_summary->bios_vendor);
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[5], out_summary->bios_version);
            break;
        case 1:
            out_summary->type1_count++;
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[4], out_summary->system_manufacturer);
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[5], out_summary->system_product);
            break;
        case 2:
            out_summary->type2_count++;
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[4], out_summary->board_manufacturer);
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[5], out_summary->board_product);
            break;
        case 4:
            out_summary->type4_count++;
            copy_string(structure + formatted_length, total_size - formatted_length,
                        structure[16], out_summary->processor_version);
            break;
        case 16: out_summary->type16_count++; break;
        case 17: out_summary->type17_count++; break;
        default: break;
        }
        offset += total_size;
    }
    return XBOOT_CORRUPT;
}
