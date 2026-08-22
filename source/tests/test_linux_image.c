#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/linux_image.h>

static uint8_t image[0x3000];

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void put64(uint8_t *p, uint64_t value) {
    put32(p, (uint32_t)value); put32(p + 4, (uint32_t)(value >> 32));
}

static void make_image(void) {
    for (size_t i = 0; i < sizeof(image); i++) image[i] = 0;
    image[0x1f1] = 4;
    put16(image + 0x1fe, 0xaa55);
    image[0x202] = 'H'; image[0x203] = 'd'; image[0x204] = 'r'; image[0x205] = 'S';
    put16(image + 0x206, 0x0215);
    put32(image + 0x1f4, 0x200);
    image[0x211] = 1;
    put32(image + 0x22c, 0x37ffffff);
    put32(image + 0x230, 0x100000);
    image[0x234] = 1;
    put16(image + 0x236, 1);
    put32(image + 0x238, 4096);
    put32(image + 0x248, 0x1000);
    put32(image + 0x24c, 0x200);
    put64(image + 0x258, 0x100000);
    put32(image + 0x260, 0x400000);
}

int main(void) {
    xboot_linux_image_info info;
    make_image();
    assert(xboot_linux_inspect(image, sizeof(image), &info) == XBOOT_OK);
    assert(info.boot_protocol == 0x0215);
    assert(info.setup_size == 0xA00);
    assert(info.payload_offset == 0x1A00 && info.payload_end == 0x1C00);
    assert(info.protected_size == 0x2000);
    assert(info.relocatable_kernel == 1 && info.is_bzimage == 1);

    make_image();
    image[0x202] = 'X';
    assert(xboot_linux_inspect(image, sizeof(image), &info) == XBOOT_PROTOCOL_ERROR);

    make_image();
    put16(image + 0x206, 0x0300);
    assert(xboot_linux_inspect(image, sizeof(image), &info) == XBOOT_UNSUPPORTED);

    make_image();
    put32(image + 0x24c, 0xfffffff0U);
    assert(xboot_linux_inspect(image, sizeof(image), &info) == XBOOT_CORRUPT);

    make_image();
    put32(image + 0x230, 3);
    assert(xboot_linux_inspect(image, sizeof(image), &info) == XBOOT_CORRUPT);
    puts("Linux bzImage inspector host tests: PASS");
    return 0;
}
