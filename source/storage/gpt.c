#include <xboot/gpt.h>

#define GPT_HEADER_SIZE_MIN 92U
#define GPT_ENTRY_SIZE_MIN 128U
#define GPT_ENTRY_SIZE_MAX 512U
#define GPT_REVISION_1_0 0x00010000U

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t read_u64(const uint8_t *p) {
    uint64_t value = 0;
    for (size_t i = 0; i < 8; i++) value |= (uint64_t)p[i] << (i * 8U);
    return value;
}

static int add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out) {
    if (b > UINT64_MAX - a) return 1;
    *out = a + b;
    return 0;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int)(crc & 1U));
    }
    return crc;
}

static int zero_guid(const uint8_t *guid) {
    uint8_t value = 0;
    for (size_t i = 0; i < 16; i++) value |= guid[i];
    return value == 0;
}

static int overlaps(const xboot_gpt_partition *a, const xboot_gpt_partition *b) {
    return !(a->last_lba < b->first_lba || b->last_lba < a->first_lba);
}

xboot_status_t xboot_gpt_scan(const xboot_block_device *device, void *scratch,
                              size_t scratch_size, xboot_gpt_summary *out_summary) {
    uint8_t *header = scratch;
    uint32_t header_crc, entry_array_crc, entry_count, entry_size;
    uint64_t entry_bytes, entry_end_bytes, entry_blocks;
    uint64_t first_usable, last_usable, entry_lba;
    uint32_t calculated;
    static uint8_t entry_buffer[GPT_ENTRY_SIZE_MAX];
    if (device == NULL || scratch == NULL || out_summary == NULL || device->read_blocks == NULL ||
        device->block_size < 512U || scratch_size < device->block_size || device->last_block < 1U) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < sizeof(*out_summary); i++) ((uint8_t *)out_summary)[i] = 0;
    if (xboot_block_read(device, 1, 1, header, scratch_size) != XBOOT_OK) return XBOOT_IO_ERROR;
    if (header[0] != 'E' || header[1] != 'F' || header[2] != 'I' || header[3] != ' ' ||
        header[4] != 'P' || header[5] != 'A' || header[6] != 'R' || header[7] != 'T') return XBOOT_NOT_FOUND;
    if (read_u32(header + 8) != GPT_REVISION_1_0) return XBOOT_PROTOCOL_ERROR;
    if (read_u32(header + 12) < GPT_HEADER_SIZE_MIN || read_u32(header + 12) > device->block_size) return XBOOT_CORRUPT;
    if (read_u32(header + 20) != 0 || read_u64(header + 24) != 1 || read_u64(header + 32) != device->last_block) return XBOOT_CORRUPT;
    first_usable = read_u64(header + 40);
    last_usable = read_u64(header + 48);
    entry_lba = read_u64(header + 72);
    entry_count = read_u32(header + 80);
    entry_size = read_u32(header + 84);
    header_crc = read_u32(header + 16);
    entry_array_crc = read_u32(header + 88);
    if (first_usable > last_usable || last_usable > device->last_block || entry_count == 0 ||
        entry_count > XBOOT_GPT_MAX_PARTITIONS || entry_size < GPT_ENTRY_SIZE_MIN ||
        entry_size > GPT_ENTRY_SIZE_MAX || (entry_size & (entry_size - 1U)) != 0) return XBOOT_CORRUPT;
    calculated = 0xFFFFFFFFU;
    for (size_t i = 0; i < read_u32(header + 12); i++) {
        uint8_t value = (i >= 16U && i < 20U) ? 0U : header[i];
        calculated = crc32_update(calculated, &value, 1);
    }
    if (~calculated != header_crc) return XBOOT_CORRUPT;
    if ((uint64_t)entry_count > UINT64_MAX / entry_size) return XBOOT_CORRUPT;
    entry_bytes = (uint64_t)entry_count * entry_size;
    if (entry_lba > UINT64_MAX / device->block_size ||
        add_overflow_u64(entry_lba * device->block_size, entry_bytes, &entry_end_bytes)) return XBOOT_CORRUPT;
    entry_blocks = (entry_bytes + device->block_size - 1U) / device->block_size;
    if (entry_blocks == 0 || entry_lba > device->last_block || entry_blocks - 1U > device->last_block - entry_lba) return XBOOT_CORRUPT;
    calculated = 0xFFFFFFFFU;
    for (uint64_t block = 0, remaining = entry_bytes; block < entry_blocks; block++) {
        size_t bytes = (remaining > device->block_size) ? device->block_size : (size_t)remaining;
        if (xboot_block_read(device, entry_lba + block, 1, scratch, scratch_size) != XBOOT_OK) return XBOOT_IO_ERROR;
        calculated = crc32_update(calculated, (const uint8_t *)scratch, bytes);
        remaining -= bytes;
    }
    if (~calculated != entry_array_crc) return XBOOT_CORRUPT;
    out_summary->disk_last_lba = device->last_block;
    out_summary->first_usable_lba = first_usable;
    out_summary->last_usable_lba = last_usable;
    out_summary->partition_entry_lba = entry_lba;
    out_summary->partition_entry_count = entry_count;
    out_summary->partition_entry_size = entry_size;
    for (uint32_t index = 0; index < entry_count; index++) {
        uint64_t entry_offset;
        xboot_gpt_partition *partition;
        if ((uint64_t)index > UINT64_MAX / entry_size ||
            add_overflow_u64(entry_lba * device->block_size, (uint64_t)index * entry_size, &entry_offset)) return XBOOT_CORRUPT;
        if (xboot_block_read_bytes(device, entry_offset, entry_size, entry_buffer, sizeof(entry_buffer)) != XBOOT_OK) return XBOOT_IO_ERROR;
        if (zero_guid(entry_buffer)) continue;
        partition = &out_summary->partitions[out_summary->active_partition_count];
        for (size_t i = 0; i < 16; i++) { partition->type_guid[i] = entry_buffer[i]; partition->unique_guid[i] = entry_buffer[16 + i]; }
        partition->first_lba = read_u64(entry_buffer + 32);
        partition->last_lba = read_u64(entry_buffer + 40);
        partition->attributes = read_u64(entry_buffer + 48);
        for (size_t i = 0; i < XBOOT_GPT_PARTITION_NAME_CHARS; i++) partition->name[i] = (uint16_t)entry_buffer[56 + i * 2] | ((uint16_t)entry_buffer[57 + i * 2] << 8);
        if (partition->first_lba > partition->last_lba || partition->first_lba < first_usable || partition->last_lba > last_usable) return XBOOT_CORRUPT;
        for (uint32_t prior = 0; prior < out_summary->active_partition_count; prior++) if (overlaps(partition, &out_summary->partitions[prior])) return XBOOT_CORRUPT;
        out_summary->active_partition_count++;
    }
    (void)entry_end_bytes;
    return XBOOT_OK;
}
