#ifndef XBOOT_LINUX_IMAGE_H
#define XBOOT_LINUX_IMAGE_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef struct {
    uint16_t boot_protocol;
    uint8_t setup_sects;
    uint32_t setup_size;
    uint32_t payload_offset;
    uint32_t payload_length;
    uint32_t cmdline_size;
    uint32_t init_size;
    uint32_t initrd_addr_max;
    uint32_t kernel_alignment;
    uint64_t pref_address;
    uint8_t relocatable_kernel;
    uint16_t xloadflags;
    uint32_t payload_end;
    uint32_t protected_size;
    uint32_t handover_offset;
    uint8_t is_bzimage;
} xboot_linux_image_info;

xboot_status_t xboot_linux_inspect(const uint8_t *image,
                                   size_t image_size,
                                   xboot_linux_image_info *out_info);

#endif
