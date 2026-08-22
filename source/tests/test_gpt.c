#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/gpt.h>

static uint8_t disk[100U * 512U];

static uint32_t crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int)(crc & 1U));
    }
    return ~crc;
}

static void put32(uint8_t *p, uint32_t value) {
    for (size_t i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (i * 8U));
}

static void put64(uint8_t *p, uint64_t value) {
    for (size_t i = 0; i < 8; i++) p[i] = (uint8_t)(value >> (i * 8U));
}

static xboot_status_t read_blocks(void *context, uint32_t media_id, uint64_t lba,
                                  size_t buffer_size, void *buffer) {
    (void)context;
    if (media_id != 5 || lba >= 100 || buffer == NULL || buffer_size != 512) return XBOOT_INVALID_ARGUMENT;
    memcpy(buffer, disk + lba * 512U, 512);
    return XBOOT_OK;
}

static void make_fixture(void) {
    uint8_t *header = disk + 512U;
    uint8_t *entry = disk + 1024U;
    memset(disk, 0, sizeof(disk));
    memcpy(header, "EFI PART", 8);
    put32(header + 8, 0x00010000U);
    put32(header + 12, 92);
    put64(header + 24, 1);
    put64(header + 32, 99);
    put64(header + 40, 34);
    put64(header + 48, 98);
    put64(header + 72, 2);
    put32(header + 80, 4);
    put32(header + 84, 128);
    for (size_t i = 0; i < 16; i++) { entry[i] = 1; entry[16 + i] = 2; }
    put64(entry + 32, 40);
    put64(entry + 40, 49);
    for (size_t i = 0; i < 5; i++) { entry[56 + i * 2] = (uint8_t)"XBOOT"[i]; entry[57 + i * 2] = 0; }
    put32(header + 88, crc32(entry, 512));
    put32(header + 16, crc32(header, 92));
}

int main(void) {
    xboot_block_device device = {disk, read_blocks, 5, 512, 99};
    xboot_gpt_summary summary;
    uint8_t scratch[512];
    make_fixture();
    assert(xboot_gpt_scan(&device, scratch, sizeof(scratch), &summary) == XBOOT_OK);
    assert(summary.active_partition_count == 1);
    assert(summary.partitions[0].first_lba == 40 && summary.partitions[0].last_lba == 49);
    disk[512U + 16U] ^= 1U;
    assert(xboot_gpt_scan(&device, scratch, sizeof(scratch), &summary) == XBOOT_CORRUPT);
    make_fixture();
    put64(disk + 1024U + 40U, 45);
    put32(disk + 512U + 88U, crc32(disk + 1024U, 512));
    put32(disk + 512U + 16U, crc32(disk + 512U, 92));
    assert(xboot_gpt_scan(&device, scratch, sizeof(scratch), &summary) == XBOOT_CORRUPT);
    puts("GPT parser host tests: PASS");
    return 0;
}
