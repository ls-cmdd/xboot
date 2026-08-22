#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/fat32.h>

static uint8_t sectors[5][512];

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static xboot_status_t read_blocks(void *context, uint32_t media_id, uint64_t lba,
                                  size_t buffer_size, void *buffer) {
    (void)context;
    if (media_id != 9 || buffer_size != 512 || lba >= 5) return XBOOT_IO_ERROR;
    for (size_t i = 0; i < 512; i++) ((uint8_t *)buffer)[i] = sectors[lba][i];
    return XBOOT_OK;
}

static void make_fixture(void) {
    for (size_t s = 0; s < 5; s++) for (size_t i = 0; i < 512; i++) sectors[s][i] = 0;
    uint8_t *bpb = sectors[0];
    put16(bpb + 11, 512);
    bpb[13] = 1;
    put16(bpb + 14, 1);
    bpb[16] = 2;
    put32(bpb + 32, 65536);
    put32(bpb + 36, 1);
    put32(bpb + 44, 2);
    put16(bpb + 510, 0xaa55);

    put32(sectors[1] + 2 * 4, 0x0fffffff);
    put32(sectors[1] + 3 * 4, 0x0fffffff);
    put32(sectors[2] + 2 * 4, 0x0fffffff);
    put32(sectors[2] + 3 * 4, 0x0fffffff);

    uint8_t *entry = sectors[3];
    const char name[11] = {'K','E','R','N','E','L',' ',' ','B','I','N'};
    for (size_t i = 0; i < 11; i++) entry[i] = (uint8_t)name[i];
    entry[11] = 0x20;
    put16(entry + 20, 0);
    put16(entry + 26, 3);
    put32(entry + 28, 5);
    sectors[4][0] = 'h'; sectors[4][1] = 'e'; sectors[4][2] = 'l';
    sectors[4][3] = 'l'; sectors[4][4] = 'o';
}

int main(void) {
    xboot_block_device device = {
        .context = NULL, .read_blocks = read_blocks, .media_id = 9,
        .block_size = 512, .last_block = 65535,
    };
    xboot_fat32_volume volume;
    xboot_fat32_file file;
    uint8_t boot[512];
    uint8_t scratch[512];
    uint8_t output[8] = {0};
    size_t bytes = 0;

    make_fixture();
    assert(xboot_fat32_mount(&device, boot, sizeof(boot), &volume) == XBOOT_OK);
    assert(volume.cluster_count == 65533);
    assert(xboot_fat32_find(&volume, "/kernel.bin", scratch, sizeof(scratch), &file) == XBOOT_OK);
    assert(file.first_cluster == 3 && file.size == 5);
    assert(xboot_fat32_read_file(&volume, &file, scratch, sizeof(scratch),
                                 output, sizeof(output), &bytes) == XBOOT_OK);
    assert(bytes == 5 && output[0] == 'h' && output[4] == 'o');
    assert(xboot_fat32_find(&volume, "/missing.bin", scratch, sizeof(scratch), &file) == XBOOT_NOT_FOUND);

    make_fixture();
    sectors[0][13] = 3;
    assert(xboot_fat32_mount(&device, boot, sizeof(boot), &volume) == XBOOT_CORRUPT);
    puts("FAT32 host tests: PASS");
    return 0;
}
