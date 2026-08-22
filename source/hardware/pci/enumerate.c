#include <xboot/pci.h>

static uint16_t read_u16(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static xboot_status_t read_config(const xboot_pci_source *source,
                                   uint16_t segment, uint8_t bus,
                                   uint8_t device, uint8_t function,
                                   uint16_t offset, void *destination,
                                   size_t size) {
    if (source == NULL || source->read == NULL || destination == NULL ||
        size == 0 || offset > 0xfffU || size > 0x1000U - offset) {
        return XBOOT_INVALID_ARGUMENT;
    }
    return source->read(source->context, segment, bus, device, function,
                        offset, destination, size);
}

static void copy_device(xboot_pci_device *destination,
                        const xboot_pci_device *source) {
    for (size_t i = 0; i < sizeof(*destination); i++) {
        ((uint8_t *)destination)[i] = ((const uint8_t *)source)[i];
    }
}

static xboot_status_t read_device(const xboot_pci_source *source,
                                  uint16_t segment, uint8_t bus,
                                  uint8_t device, uint8_t function,
                                  xboot_pci_device *out_device) {
    uint8_t header[0x40];
    xboot_status_t status;
    status = read_config(source, segment, bus, device, function,
                         0, header, sizeof(header));
    if (status != XBOOT_OK) return status;
    out_device->segment = segment;
    out_device->bus = bus;
    out_device->device = device;
    out_device->function = function;
    out_device->vendor_id = read_u16(header + 0x00);
    out_device->device_id = read_u16(header + 0x02);
    out_device->revision_id = header[0x08];
    out_device->programming_interface = header[0x09];
    out_device->subclass = header[0x0a];
    out_device->class_code = header[0x0b];
    out_device->header_type = header[0x0e];
    out_device->multifunction = (uint8_t)((header[0x0e] >> 7) & 1);
    for (size_t i = 0; i < 6; i++) out_device->bars[i] = read_u32(header + 0x10 + i * 4);
    return XBOOT_OK;
}

xboot_status_t xboot_pci_enumerate(const xboot_pci_source *source,
                                   uint16_t segment, uint8_t first_bus,
                                   uint8_t last_bus, xboot_pci_graph *out_graph) {
    if (source == NULL || source->read == NULL || out_graph == NULL ||
        first_bus > last_bus) return XBOOT_INVALID_ARGUMENT;
    out_graph->device_count = 0;
    for (size_t i = 0; i < XBOOT_PCI_MAX_DEVICES; i++) {
        for (size_t j = 0; j < sizeof(out_graph->devices[i]); j++) {
            ((uint8_t *)&out_graph->devices[i])[j] = 0;
        }
    }
    for (uint16_t bus = first_bus; bus <= last_bus; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            xboot_pci_device function_zero;
            xboot_status_t status = read_device(source, segment, (uint8_t)bus,
                                                device, 0, &function_zero);
            if (status != XBOOT_OK) return status;
            if (function_zero.vendor_id == 0xffffU) continue;
            if (out_graph->device_count >= XBOOT_PCI_MAX_DEVICES) return XBOOT_NO_MEMORY;
            copy_device(&out_graph->devices[out_graph->device_count++], &function_zero);
            uint8_t function_limit = function_zero.multifunction ? 8 : 1;
            for (uint8_t function = 1; function < function_limit; function++) {
                xboot_pci_device candidate;
                status = read_device(source, segment, (uint8_t)bus, device,
                                     function, &candidate);
                if (status != XBOOT_OK) return status;
                if (candidate.vendor_id == 0xffffU) continue;
                if (out_graph->device_count >= XBOOT_PCI_MAX_DEVICES) return XBOOT_NO_MEMORY;
                copy_device(&out_graph->devices[out_graph->device_count++], &candidate);
            }
        }
        if (bus == 0xff) break;
    }
    return XBOOT_OK;
}
