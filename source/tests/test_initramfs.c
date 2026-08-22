#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xboot/initramfs.h>

static uint8_t archive[512];

static void hex8(uint8_t *p, uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    for (int i = 7; i >= 0; i--) { p[i] = (uint8_t)digits[value & 0xfU]; value >>= 4; }
}

static size_t align4(size_t value) { return (value + 3U) & ~3U; }

static size_t add_entry(size_t offset, const char *magic, const char *name,
                        const uint8_t *data, size_t data_size, uint32_t checksum) {
    uint8_t *header = archive + offset;
    for (size_t i = 0; i < 110; i++) header[i] = '0';
    for (size_t i = 0; i < 6; i++) header[i] = (uint8_t)magic[i];
    hex8(header + 14, 0100644);
    hex8(header + 54, (uint32_t)data_size);
    hex8(header + 94, (uint32_t)strlen(name) + 1U);
    hex8(header + 102, checksum);
    size_t name_offset = offset + 110;
    for (size_t i = 0; i < strlen(name); i++) archive[name_offset + i] = (uint8_t)name[i];
    archive[name_offset + strlen(name)] = 0;
    size_t data_offset = align4(name_offset + strlen(name) + 1);
    for (size_t i = 0; i < data_size; i++) archive[data_offset + i] = data[i];
    return align4(data_offset + data_size);
}

static xboot_status_t collect(void *context, const xboot_initramfs_entry *entry) {
    uint32_t *count = context;
    (*count)++;
    assert(entry->name[0] == 'h');
    assert(entry->filesize == 5);
    return XBOOT_OK;
}

int main(void) {
    const uint8_t data[] = {'h','e','l','l','o'};
    uint32_t count = 0;
    uint32_t entries = 0;
    size_t offset;
    for (size_t i = 0; i < sizeof(archive); i++) archive[i] = 0;
    offset = add_entry(0, "070701", "hello", data, sizeof(data), 0);
    offset = add_entry(offset, "070701", "TRAILER!!!", NULL, 0, 0);
    assert(xboot_initramfs_inspect(archive, offset, collect, &count, &entries) == XBOOT_OK);
    assert(count == 1 && entries == 1);

    archive[0] = 'X';
    assert(xboot_initramfs_inspect(archive, offset, NULL, NULL, &entries) == XBOOT_PROTOCOL_ERROR);

    for (size_t i = 0; i < sizeof(archive); i++) archive[i] = 0;
    offset = add_entry(0, "070702", "hello", data, sizeof(data), 1);
    offset = add_entry(offset, "070701", "TRAILER!!!", NULL, 0, 0);
    assert(xboot_initramfs_inspect(archive, offset, NULL, NULL, &entries) == XBOOT_CORRUPT);
    puts("initramfs inspector host tests: PASS");
    return 0;
}
