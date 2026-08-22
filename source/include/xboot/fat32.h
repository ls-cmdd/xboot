#ifndef XBOOT_FAT32_H
#define XBOOT_FAT32_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/storage.h>

typedef struct {
    const xboot_block_device *device;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t sectors_per_fat;
    uint32_t root_cluster;
    uint64_t fat_start_lba;
    uint64_t data_start_lba;
    uint32_t cluster_count;
} xboot_fat32_volume;

typedef struct {
    uint32_t first_cluster;
    uint32_t size;
    uint8_t attributes;
} xboot_fat32_file;

xboot_status_t xboot_fat32_mount(const xboot_block_device *device,
                                 uint8_t *boot_sector,
                                 size_t boot_sector_size,
                                 xboot_fat32_volume *out_volume);

xboot_status_t xboot_fat32_find(const xboot_fat32_volume *volume,
                                const char *path,
                                uint8_t *scratch,
                                size_t scratch_size,
                                xboot_fat32_file *out_file);

xboot_status_t xboot_fat32_read_file(const xboot_fat32_volume *volume,
                                     const xboot_fat32_file *file,
                                     uint8_t *scratch,
                                     size_t scratch_size,
                                     uint8_t *output,
                                     size_t output_size,
                                     size_t *out_bytes);

#endif
