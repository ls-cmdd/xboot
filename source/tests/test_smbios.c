#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/smbios.h>

#define MEM_SIZE 0x1000
static uint8_t image[MEM_SIZE];

static xboot_status_t read_image(void *context, uint64_t address,
                                  void *destination, size_t size) {
    uint8_t *bytes = context;
    if (address > MEM_SIZE || size > MEM_SIZE - (size_t)address) return XBOOT_IO_ERROR;
    for (size_t i = 0; i < size; i++) ((uint8_t *)destination)[i] = bytes[address + i];
    return XBOOT_OK;
}

static uint8_t checksum(const uint8_t *bytes, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) sum = (uint8_t)(sum + bytes[i]);
    return sum;
}

static void fix_checksum(uint8_t *bytes, size_t length, size_t checksum_offset) {
    bytes[checksum_offset] = 0;
    bytes[checksum_offset] = (uint8_t)(0U - checksum(bytes, length));
}

static void put_u32(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void put_u64(uint8_t *bytes, uint64_t value) {
    put_u32(bytes, (uint32_t)value);
    put_u32(bytes + 4, (uint32_t)(value >> 32));
}

static void make_fixture(void) {
    uint8_t *entry = image + 0x100;
    uint8_t *table = image + 0x300;
    for (size_t i = 0; i < sizeof(image); i++) image[i] = 0;
    entry[0] = '_'; entry[1] = 'S'; entry[2] = 'M'; entry[3] = '3'; entry[4] = '_';
    entry[6] = 0x18;
    entry[7] = 3; entry[8] = 8; entry[9] = 0;
    put_u32(entry + 12, 32);
    put_u64(entry + 16, 0x300);

    table[0] = 0; table[1] = 6; table[2] = 0; table[3] = 0;
    table[4] = 1; table[5] = 2;
    table[6] = 'B'; table[7] = 'I'; table[8] = 'O'; table[9] = 'S'; table[10] = 0;
    table[11] = '1'; table[12] = '.'; table[13] = '0'; table[14] = 0; table[15] = 0;
    table[16] = 127; table[17] = 4; table[18] = 0; table[19] = 0;
    table[20] = 0; table[21] = 0;
    fix_checksum(entry, 0x18, 5);
}

int main(void) {
    xboot_smbios_source source = {.context = image, .read = read_image};
    xboot_smbios_summary summary;

    make_fixture();
    assert(xboot_smbios_parse(&source, 0x100, &summary) == XBOOT_OK);
    assert(summary.major == 3 && summary.minor == 8);
    assert(summary.table_address == 0x300 && summary.table_length == 32);
    assert(summary.structure_count == 2);
    assert(summary.type0_count == 1);
    assert(summary.bios_vendor[0] == 'B');

    make_fixture();
    image[0x100 + 5] ^= 1;
    assert(xboot_smbios_parse(&source, 0x100, &summary) == XBOOT_CORRUPT);

    make_fixture();
    image[0x300 + 17] = 3;
    assert(xboot_smbios_parse(&source, 0x100, &summary) == XBOOT_CORRUPT);

    assert(xboot_smbios_parse(&source, 0, &summary) == XBOOT_INVALID_ARGUMENT);
    puts("SMBIOS parser host tests: PASS");
    return 0;
}
