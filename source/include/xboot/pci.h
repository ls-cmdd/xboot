#ifndef XBOOT_PCI_H
#define XBOOT_PCI_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef xboot_status_t (*xboot_pci_config_read)(void *context,
                                                uint16_t segment,
                                                uint8_t bus,
                                                uint8_t device,
                                                uint8_t function,
                                                uint16_t offset,
                                                void *destination,
                                                size_t size);

typedef struct {
    void *context;
    xboot_pci_config_read read;
} xboot_pci_source;

typedef struct {
    uint16_t segment;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t programming_interface;
    uint8_t header_type;
    uint8_t multifunction;
    uint32_t bars[6];
} xboot_pci_device;

#define XBOOT_PCI_MAX_DEVICES 256U

typedef struct {
    xboot_pci_device devices[XBOOT_PCI_MAX_DEVICES];
    size_t device_count;
} xboot_pci_graph;

xboot_status_t xboot_pci_enumerate(const xboot_pci_source *source,
                                   uint16_t segment,
                                   uint8_t first_bus,
                                   uint8_t last_bus,
                                   xboot_pci_graph *out_graph);

#endif
