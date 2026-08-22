#include <xboot/storage.h>

static uint8_t g_byte_read_scratch[4096];

xboot_status_t xboot_block_read(const xboot_block_device *device,
                                uint64_t lba, uint64_t block_count,
                                void *buffer, size_t buffer_size) {
    uint64_t end_lba;
    uint64_t bytes64;
    if (device == NULL || device->read_blocks == NULL || buffer == NULL ||
        device->block_size == 0 || block_count == 0 ||
        block_count > UINT64_MAX / device->block_size) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if (lba > device->last_block || block_count - 1 > device->last_block - lba) {
        return XBOOT_IO_ERROR;
    }
    end_lba = lba + block_count - 1;
    (void)end_lba;
    bytes64 = block_count * device->block_size;
    if (bytes64 > SIZE_MAX || (size_t)bytes64 != buffer_size) {
        return XBOOT_INVALID_ARGUMENT;
    }
    return device->read_blocks(device->context, device->media_id, lba,
                               buffer_size, buffer);
}

xboot_status_t xboot_block_read_bytes(const xboot_block_device *device,
                                      uint64_t byte_offset, size_t size,
                                      void *buffer, size_t buffer_size) {
    uint64_t first_lba;
    uint64_t last_byte;
    uint64_t last_lba;
    uint64_t block_count;
    uint64_t offset_in_block;
    uint64_t total_bytes;
    uint8_t *destination = buffer;
    if (device == NULL || buffer == NULL || size == 0 || size > buffer_size ||
        device->block_size == 0 || device->block_size > sizeof(g_byte_read_scratch)) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if ((uint64_t)size > UINT64_MAX - byte_offset) return XBOOT_INVALID_ARGUMENT;
    last_byte = byte_offset + size - 1;
    if (last_byte / device->block_size > device->last_block) return XBOOT_IO_ERROR;
    first_lba = byte_offset / device->block_size;
    last_lba = last_byte / device->block_size;
    block_count = last_lba - first_lba + 1;
    offset_in_block = byte_offset % device->block_size;
    total_bytes = block_count * device->block_size;
    if (total_bytes > sizeof(g_byte_read_scratch)) {
        return XBOOT_BUFFER_TOO_SMALL;
    }
    if (xboot_block_read(device, first_lba, block_count,
                         g_byte_read_scratch, (size_t)total_bytes) != XBOOT_OK) {
        return XBOOT_IO_ERROR;
    }
    for (size_t i = 0; i < size; i++) destination[i] = g_byte_read_scratch[offset_in_block + i];
    return XBOOT_OK;
}
