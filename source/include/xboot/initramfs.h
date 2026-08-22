#ifndef XBOOT_INITRAMFS_H
#define XBOOT_INITRAMFS_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t filesize;
    uint32_t namesize;
    uint32_t checksum;
    uint8_t is_crc;
    uint8_t is_trailer;
    const uint8_t *name;
    const uint8_t *data;
} xboot_initramfs_entry;

typedef xboot_status_t (*xboot_initramfs_entry_callback)(void *context,
                                                         const xboot_initramfs_entry *entry);

xboot_status_t xboot_initramfs_inspect(const uint8_t *archive,
                                       size_t archive_size,
                                       xboot_initramfs_entry_callback callback,
                                       void *context,
                                       uint32_t *out_entry_count);

#endif
