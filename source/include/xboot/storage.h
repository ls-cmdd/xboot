#ifndef XBOOT_STORAGE_H
#define XBOOT_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef xboot_status_t (*xboot_block_read_blocks)(void *context,
                                                  uint32_t media_id,
                                                  uint64_t lba,
                                                  size_t buffer_size,
                                                  void *buffer);

typedef struct {
    void *context;
    xboot_block_read_blocks read_blocks;
    uint32_t media_id;
    uint32_t block_size;
    uint64_t last_block;
} xboot_block_device;

xboot_status_t xboot_block_read(const xboot_block_device *device,
                                uint64_t lba,
                                uint64_t block_count,
                                void *buffer,
                                size_t buffer_size);

xboot_status_t xboot_block_read_bytes(const xboot_block_device *device,
                                      uint64_t byte_offset,
                                      size_t size,
                                      void *buffer,
                                      size_t buffer_size);

#endif
