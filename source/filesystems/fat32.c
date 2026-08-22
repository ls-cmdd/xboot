#include <xboot/fat32.h>

#define FAT32_EOC 0x0ffffff8U
#define FAT32_BAD 0x0ffffff7U
#define FAT32_MAX_COMPONENT 11U

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int ascii_equal_fold(uint8_t a, uint8_t b) {
    if (a >= 'a' && a <= 'z') a = (uint8_t)(a - 'a' + 'A');
    if (b >= 'a' && b <= 'z') b = (uint8_t)(b - 'a' + 'A');
    return a == b;
}

static xboot_status_t read_cluster(const xboot_fat32_volume *volume,
                                   uint32_t cluster, uint8_t *buffer,
                                   size_t buffer_size) {
    uint64_t lba;
    uint64_t cluster_bytes;
    if (cluster < 2 || cluster >= FAT32_EOC || volume == NULL ||
        volume->device == NULL || buffer == NULL ||
        volume->sectors_per_cluster == 0 ||
        volume->sectors_per_cluster > UINT64_MAX / volume->bytes_per_sector) {
        return XBOOT_INVALID_ARGUMENT;
    }
    cluster_bytes = (uint64_t)volume->sectors_per_cluster * volume->bytes_per_sector;
    if (cluster_bytes > buffer_size || cluster > volume->cluster_count + 1U ||
        (uint64_t)(cluster - 2U) > UINT64_MAX / volume->sectors_per_cluster) {
        return XBOOT_BUFFER_TOO_SMALL;
    }
    lba = volume->data_start_lba + (uint64_t)(cluster - 2U) * volume->sectors_per_cluster;
    return xboot_block_read(volume->device, lba, volume->sectors_per_cluster,
                            buffer, (size_t)cluster_bytes);
}

static xboot_status_t next_cluster(const xboot_fat32_volume *volume,
                                   uint32_t cluster, uint8_t *scratch,
                                   size_t scratch_size, uint32_t *out_next) {
    uint64_t fat_offset;
    uint64_t fat_lba;
    uint32_t in_sector;
    uint32_t value;
    if (volume == NULL || scratch == NULL || out_next == NULL ||
        volume->bytes_per_sector == 0 || volume->bytes_per_sector > scratch_size ||
        cluster < 2 || cluster >= volume->cluster_count + 2U) return XBOOT_INVALID_ARGUMENT;
    fat_offset = (uint64_t)cluster * 4U;
    fat_lba = volume->fat_start_lba + fat_offset / volume->bytes_per_sector;
    in_sector = (uint32_t)(fat_offset % volume->bytes_per_sector);
    if (in_sector + 4U > volume->bytes_per_sector) return XBOOT_CORRUPT;
    if (xboot_block_read(volume->device, fat_lba, 1, scratch,
                         volume->bytes_per_sector) != XBOOT_OK) return XBOOT_IO_ERROR;
    value = read_u32(scratch + in_sector) & 0x0fffffffU;
    if (value == FAT32_BAD || value == 0 || value == 1 || value >= volume->cluster_count + 2U) {
        if (value >= FAT32_EOC) {
            *out_next = FAT32_EOC;
            return XBOOT_OK;
        }
        return XBOOT_CORRUPT;
    }
    *out_next = value;
    return XBOOT_OK;
}

static xboot_status_t make_83(const char **path, char name[12], int *out_last) {
    const char *p = *path;
    size_t base_len = 0;
    size_t ext_len = 0;
    int in_extension = 0;
    for (size_t i = 0; i < 11; i++) name[i] = ' ';
    name[11] = '\0';
    while (*p == '/' || *p == '\\') p++;
    if (*p == '\0') return XBOOT_NOT_FOUND;
    while (*p != '\0' && *p != '/' && *p != '\\') {
        if (*p == '.') {
            if (in_extension) return XBOOT_CORRUPT;
            in_extension = 1;
            p++;
            continue;
        }
        if (*p < 0x21 || *p > 0x7e) return XBOOT_CORRUPT;
        if (!in_extension) {
            if (base_len >= 8) return XBOOT_CORRUPT;
            name[base_len++] = (char)(*p >= 'a' && *p <= 'z' ? *p - 'a' + 'A' : *p);
        } else {
            if (ext_len >= 3) return XBOOT_CORRUPT;
            name[8 + ext_len++] = (char)(*p >= 'a' && *p <= 'z' ? *p - 'a' + 'A' : *p);
        }
        p++;
    }
    if (base_len == 0) return XBOOT_CORRUPT;
    while (*p == '/' || *p == '\\') p++;
    *out_last = *p == '\0';
    *path = p;
    return XBOOT_OK;
}

static xboot_status_t find_in_directory(const xboot_fat32_volume *volume,
                                        uint32_t directory_cluster,
                                        const char wanted[12], uint8_t *scratch,
                                        size_t scratch_size, xboot_fat32_file *out_file) {
    uint32_t cluster = directory_cluster;
    uint32_t max_steps = volume->cluster_count + 1U;
    for (uint32_t step = 0; step < max_steps; step++) {
        xboot_status_t status = read_cluster(volume, cluster, scratch, scratch_size);
        if (status != XBOOT_OK) return status;
        size_t cluster_bytes = (size_t)volume->bytes_per_sector * volume->sectors_per_cluster;
        for (size_t offset = 0; offset + 32 <= cluster_bytes; offset += 32) {
            uint8_t first = scratch[offset];
            if (first == 0) return XBOOT_NOT_FOUND;
            if (first == 0xe5 || scratch[offset + 11] == 0x0f) continue;
            int match = 1;
            for (size_t i = 0; i < FAT32_MAX_COMPONENT; i++) {
                if (!ascii_equal_fold(first == 0 && i == 0 ? first : scratch[offset + i],
                                      (uint8_t)wanted[i])) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                uint32_t low = read_u16(scratch + offset + 26);
                uint32_t high = read_u16(scratch + offset + 20);
                out_file->first_cluster = (high << 16) | low;
                out_file->size = read_u32(scratch + offset + 28);
                out_file->attributes = scratch[offset + 11];
                return XBOOT_OK;
            }
        }
        status = next_cluster(volume, cluster, scratch, scratch_size, &cluster);
        if (status != XBOOT_OK || cluster >= FAT32_EOC) return status == XBOOT_OK ? XBOOT_NOT_FOUND : status;
    }
    return XBOOT_CORRUPT;
}

xboot_status_t xboot_fat32_mount(const xboot_block_device *device,
                                 uint8_t *boot_sector, size_t boot_sector_size,
                                 xboot_fat32_volume *out_volume) {
    uint32_t total_sectors;
    uint32_t fat_size;
    uint32_t data_sectors;
    if (device == NULL || boot_sector == NULL || out_volume == NULL ||
        boot_sector_size < 512 || device->block_size != 512) return XBOOT_INVALID_ARGUMENT;
    if (xboot_block_read(device, 0, 1, boot_sector, 512) != XBOOT_OK) return XBOOT_IO_ERROR;
    if (read_u16(boot_sector + 510) != 0xaa55 ||
        read_u16(boot_sector + 11) != 512 || boot_sector[13] == 0 ||
        read_u16(boot_sector + 14) == 0 || boot_sector[16] == 0 ||
        read_u32(boot_sector + 36) == 0 || read_u32(boot_sector + 44) < 2) {
        return XBOOT_CORRUPT;
    }
    total_sectors = read_u16(boot_sector + 19);
    if (total_sectors == 0) total_sectors = read_u32(boot_sector + 32);
    fat_size = read_u32(boot_sector + 36);
    uint64_t metadata_sectors = (uint64_t)read_u16(boot_sector + 14) +
                                (uint64_t)boot_sector[16] * fat_size;
    if (total_sectors <= metadata_sectors || metadata_sectors > UINT32_MAX) {
        return XBOOT_CORRUPT;
    }
    data_sectors = total_sectors - (uint32_t)metadata_sectors;
    out_volume->device = device;
    out_volume->bytes_per_sector = 512;
    out_volume->sectors_per_cluster = boot_sector[13];
    out_volume->reserved_sectors = read_u16(boot_sector + 14);
    out_volume->fat_count = boot_sector[16];
    out_volume->sectors_per_fat = fat_size;
    out_volume->root_cluster = read_u32(boot_sector + 44);
    out_volume->fat_start_lba = out_volume->reserved_sectors;
    out_volume->data_start_lba = out_volume->fat_start_lba +
                                 (uint64_t)out_volume->fat_count * fat_size;
    out_volume->cluster_count = data_sectors / out_volume->sectors_per_cluster;
    if (out_volume->cluster_count < 65525 || out_volume->cluster_count > 0x0ffffff5U ||
        out_volume->data_start_lba > device->last_block ||
        (uint64_t)out_volume->cluster_count * out_volume->sectors_per_cluster >
            device->last_block - out_volume->data_start_lba + 1U) {
        return XBOOT_CORRUPT;
    }
    return XBOOT_OK;
}

xboot_status_t xboot_fat32_find(const xboot_fat32_volume *volume, const char *path,
                                uint8_t *scratch, size_t scratch_size,
                                xboot_fat32_file *out_file) {
    uint32_t current;
    if (volume == NULL || path == NULL || scratch == NULL || out_file == NULL) return XBOOT_INVALID_ARGUMENT;
    current = volume->root_cluster;
    for (;;) {
        char wanted[12];
        int last;
        xboot_status_t status = make_83(&path, wanted, &last);
        if (status != XBOOT_OK) return status;
        status = find_in_directory(volume, current, wanted, scratch, scratch_size, out_file);
        if (status != XBOOT_OK) return status;
        if (last) return XBOOT_OK;
        if ((out_file->attributes & 0x10) == 0 || out_file->first_cluster < 2) return XBOOT_CORRUPT;
        current = out_file->first_cluster;
    }
}

xboot_status_t xboot_fat32_read_file(const xboot_fat32_volume *volume,
                                     const xboot_fat32_file *file, uint8_t *scratch,
                                     size_t scratch_size, uint8_t *output,
                                     size_t output_size, size_t *out_bytes) {
    uint32_t cluster;
    size_t remaining;
    size_t written = 0;
    if (volume == NULL || file == NULL || scratch == NULL || output == NULL ||
        out_bytes == NULL || file->size > output_size) return XBOOT_INVALID_ARGUMENT;
    cluster = file->first_cluster;
    remaining = file->size;
    while (remaining > 0) {
        xboot_status_t status;
        size_t cluster_bytes = (size_t)volume->bytes_per_sector * volume->sectors_per_cluster;
        status = read_cluster(volume, cluster, scratch, scratch_size);
        if (status != XBOOT_OK) return status;
        size_t copy = remaining < cluster_bytes ? remaining : cluster_bytes;
        for (size_t i = 0; i < copy; i++) output[written + i] = scratch[i];
        written += copy;
        remaining -= copy;
        if (remaining == 0) break;
        status = next_cluster(volume, cluster, scratch, scratch_size, &cluster);
        if (status != XBOOT_OK || cluster >= FAT32_EOC) return XBOOT_CORRUPT;
    }
    *out_bytes = written;
    return XBOOT_OK;
}
