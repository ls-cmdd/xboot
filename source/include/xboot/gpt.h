#ifndef XBOOT_GPT_H
#define XBOOT_GPT_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/storage.h>

#define XBOOT_GPT_MAX_PARTITIONS 128U
#define XBOOT_GPT_PARTITION_NAME_CHARS 36U

typedef struct {
    uint8_t type_guid[16];
    uint8_t unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[XBOOT_GPT_PARTITION_NAME_CHARS];
} xboot_gpt_partition;

typedef struct {
    uint64_t disk_last_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint64_t partition_entry_lba;
    uint32_t partition_entry_count;
    uint32_t partition_entry_size;
    uint32_t active_partition_count;
    xboot_gpt_partition partitions[XBOOT_GPT_MAX_PARTITIONS];
} xboot_gpt_summary;

xboot_status_t xboot_gpt_scan(const xboot_block_device *device,
                              void *scratch,
                              size_t scratch_size,
                              xboot_gpt_summary *out_summary);

#endif
