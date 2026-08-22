#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/storage.h>

static uint8_t disk[8][512];

static xboot_status_t read_blocks(void *context, uint32_t media_id, uint64_t lba,
                                  size_t buffer_size, void *buffer) {
    (void)context;
    if (media_id != 7 || lba >= 8 || buffer_size == 0 ||
        buffer_size % 512 != 0 || buffer_size / 512 > 8 - lba) return XBOOT_IO_ERROR;
    for (size_t block = 0; block < buffer_size / 512; block++) {
        for (size_t i = 0; i < 512; i++) {
            ((uint8_t *)buffer)[block * 512 + i] = disk[lba + block][i];
        }
    }
    return XBOOT_OK;
}

int main(void) {
    xboot_block_device device = {
        .context = NULL,
        .read_blocks = read_blocks,
        .media_id = 7,
        .block_size = 512,
        .last_block = 7,
    };
    uint8_t block[512];
    uint8_t bytes[40];
    for (size_t b = 0; b < 8; b++) {
        for (size_t i = 0; i < 512; i++) disk[b][i] = (uint8_t)(b + i);
    }
    assert(xboot_block_read(&device, 2, 1, block, sizeof(block)) == XBOOT_OK);
    assert(block[0] == 2 && block[10] == 12);
    assert(xboot_block_read_bytes(&device, 500, sizeof(bytes), bytes, sizeof(bytes)) == XBOOT_OK);
    assert(bytes[0] == (uint8_t)(500 % 256));
    assert(bytes[11] == (uint8_t)(511 % 256));
    assert(bytes[12] == 1);
    assert(xboot_block_read(&device, 8, 1, block, sizeof(block)) == XBOOT_IO_ERROR);
    assert(xboot_block_read(&device, 0, 1, block, sizeof(block) - 1) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_block_read_bytes(&device, 4096, 1, bytes, sizeof(bytes)) == XBOOT_IO_ERROR);
    puts("storage abstraction host tests: PASS");
    return 0;
}
