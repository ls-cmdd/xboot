#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/acpi.h>

#define MEM_SIZE 0x1000
static uint8_t memory_image[MEM_SIZE];

static xboot_status_t memory_read(void *context, uint64_t address,
                                  void *destination, size_t size) {
    uint8_t *memory = context;
    if (address > MEM_SIZE || size > MEM_SIZE - (size_t)address) {
        return XBOOT_IO_ERROR;
    }
    for (size_t i = 0; i < size; i++) {
        ((uint8_t *)destination)[i] = memory[address + i];
    }
    return XBOOT_OK;
}

static void checksum(uint8_t *bytes, size_t length, size_t checksum_offset) {
    uint8_t sum = 0;
    bytes[checksum_offset] = 0;
    for (size_t i = 0; i < length; i++) {
        sum = (uint8_t)(sum + bytes[i]);
    }
    bytes[checksum_offset] = (uint8_t)(0U - sum);
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

static void make_header(uint64_t address, const char signature[4], uint32_t length) {
    uint8_t *header = memory_image + address;
    for (size_t i = 0; i < 4; i++) header[i] = (uint8_t)signature[i];
    put_u32(header + 4, length);
    header[8] = 1;
    checksum(header, length, 9);
}

static void make_fixture(void) {
    uint8_t *rsdp = memory_image + 0x100;
    uint8_t *xsdt = memory_image + 0x200;
    for (size_t i = 0; i < sizeof(memory_image); i++) memory_image[i] = 0;
    for (size_t i = 0; i < 8; i++) rsdp[i] = (uint8_t)"RSD PTR "[i];
    rsdp[15] = 2;
    put_u32(rsdp + 20, 36);
    put_u64(rsdp + 24, 0x200);
    checksum(rsdp, 20, 8);
    checksum(rsdp, 36, 32);
    make_header(0x200, "XSDT", 52);
    put_u64(xsdt + 36, 0x300);
    put_u64(xsdt + 44, 0x380);
    make_header(0x300, "APIC", 44);
    make_header(0x380, "MCFG", 60);
    put_u64(memory_image + 0x380 + 44, 0x100000);
    memory_image[0x380 + 44 + 10] = 0;
    memory_image[0x380 + 44 + 11] = 0;
    checksum(memory_image + 0x380, 60, 9);
    checksum(xsdt, 52, 9);
}

int main(void) {
    xboot_acpi_source source = {.context = memory_image, .read = memory_read};
    xboot_acpi_summary summary;

    make_fixture();
    assert(xboot_acpi_parse(&source, 0x100, &summary) == XBOOT_OK);
    assert(summary.revision == 2);
    assert(summary.root_table_address == 0x200);
    assert(summary.table_count == 2);
    assert(summary.madt_count == 1);
    assert(summary.mcfg_count == 1);
    assert(summary.mcfg_segment_count == 1);
    assert(summary.mcfg_base_address == 0x100000);

    make_fixture();
    memory_image[0x100 + 8] ^= 1;
    assert(xboot_acpi_parse(&source, 0x100, &summary) == XBOOT_CORRUPT);

    make_fixture();
    memory_image[0x300 + 9] ^= 1;
    assert(xboot_acpi_parse(&source, 0x100, &summary) == XBOOT_CORRUPT);

    make_fixture();
    put_u32(memory_image + 0x200 + 4, 0xffffffffU);
    assert(xboot_acpi_parse(&source, 0x100, &summary) == XBOOT_CORRUPT);

    assert(xboot_acpi_parse(&source, 0, &summary) == XBOOT_INVALID_ARGUMENT);
    puts("ACPI parser host tests: PASS");
    return 0;
}
