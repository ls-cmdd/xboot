#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/pci.h>

static uint8_t config[32][8][64];

static uint16_t read_u16(uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static void put_u16(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static xboot_status_t read_config(void *context, uint16_t segment, uint8_t bus,
                                  uint8_t device, uint8_t function,
                                  uint16_t offset, void *destination, size_t size) {
    (void)segment;
    if (context == NULL || bus != 0 || device >= 32 || function >= 8 ||
        offset > 64 || size > 64 - offset) return XBOOT_IO_ERROR;
    for (size_t i = 0; i < size; i++) {
        ((uint8_t *)destination)[i] = config[device][function][offset + i];
    }
    return XBOOT_OK;
}

static void make_fixture(void) {
    for (size_t d = 0; d < 32; d++) {
        for (size_t f = 0; f < 8; f++) {
            for (size_t i = 0; i < 64; i++) config[d][f][i] = 0xff;
        }
    }
    put_u16(config[1][0] + 0, 0x1234);
    put_u16(config[1][0] + 2, 0x5678);
    config[1][0][8] = 1;
    config[1][0][9] = 2;
    config[1][0][10] = 8;
    config[1][0][11] = 1;
    config[1][0][14] = 0;
    config[1][0][0x10] = 0x00;
    config[1][0][0x11] = 0x10;

    put_u16(config[2][0] + 0, 0xabcd);
    put_u16(config[2][0] + 2, 0x1000);
    config[2][0][14] = 0x80;
    put_u16(config[2][1] + 0, 0xdcba);
    put_u16(config[2][1] + 2, 0x1001);
    config[2][1][14] = 0;
}

int main(void) {
    xboot_pci_source source = {.context = config, .read = read_config};
    xboot_pci_graph graph;

    make_fixture();
    assert(xboot_pci_enumerate(&source, 0, 0, 0, &graph) == XBOOT_OK);
    assert(graph.device_count == 3);
    assert(graph.devices[0].vendor_id == 0x1234);
    assert(graph.devices[0].class_code == 1);
    assert(graph.devices[1].vendor_id == 0xabcd);
    assert(graph.devices[2].function == 1);
    assert(read_u16((uint8_t *)&graph.devices[2].vendor_id) == 0xdcba);

    assert(xboot_pci_enumerate(&source, 0, 2, 1, &graph) == XBOOT_INVALID_ARGUMENT);
    puts("PCI enumerator host tests: PASS");
    return 0;
}
