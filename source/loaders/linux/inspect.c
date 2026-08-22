#include <xboot/linux_image.h>

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t read_u64(const uint8_t *p) {
    return (uint64_t)read_u32(p) | ((uint64_t)read_u32(p + 4) << 32);
}

xboot_status_t xboot_linux_inspect(const uint8_t *image, size_t image_size,
                                   xboot_linux_image_info *out_info) {
    uint32_t setup_bytes;
    uint64_t payload_end;
    uint64_t protected_size;
    uint64_t protected_end;
    uint32_t setup_sects;
    if (image == NULL || out_info == NULL || image_size < 0x270) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < sizeof(*out_info); i++) ((uint8_t *)out_info)[i] = 0;
    if (read_u16(image + 0x1fe) != 0xaa55 ||
        image[0x202] != 'H' || image[0x203] != 'd' ||
        image[0x204] != 'r' || image[0x205] != 'S') {
        return XBOOT_PROTOCOL_ERROR;
    }
    out_info->boot_protocol = read_u16(image + 0x206);
    if (out_info->boot_protocol < 0x0200 || out_info->boot_protocol > 0x0215) {
        return XBOOT_UNSUPPORTED;
    }
    setup_sects = image[0x1f1] == 0 ? 4U : image[0x1f1];
    setup_bytes = (setup_sects + 1U) * 512U;
    if (setup_bytes < 0x270 || setup_bytes > image_size) return XBOOT_CORRUPT;
    out_info->setup_sects = (uint8_t)setup_sects;
    out_info->setup_size = setup_bytes;
    if (out_info->boot_protocol >= 0x0208) {
        uint32_t payload_relative = read_u32(image + 0x248);
        if (payload_relative > UINT32_MAX - setup_bytes) return XBOOT_CORRUPT;
        out_info->payload_offset = payload_relative == 0 ? setup_bytes : setup_bytes + payload_relative;
    } else {
        out_info->payload_offset = setup_bytes;
    }
    out_info->payload_length = out_info->boot_protocol >= 0x0208 ? read_u32(image + 0x24c) : 0;
    protected_size = out_info->boot_protocol >= 0x0204 ? (uint64_t)read_u32(image + 0x1f4) * 16ULL : (uint64_t)image_size - setup_bytes;
    if (protected_size == 0 || protected_size > (uint64_t)image_size - setup_bytes) return XBOOT_CORRUPT;
    protected_end = (uint64_t)setup_bytes + protected_size;
    out_info->protected_size = (uint32_t)protected_size;
    out_info->handover_offset = out_info->boot_protocol >= 0x020b ? read_u32(image + 0x264) : 0;
    if (out_info->handover_offset != 0 && out_info->handover_offset >= image_size) return XBOOT_CORRUPT;
    out_info->initrd_addr_max = out_info->boot_protocol >= 0x0203 ? read_u32(image + 0x22c) : 0x37ffffffU;
    out_info->kernel_alignment = out_info->boot_protocol >= 0x0205 ? read_u32(image + 0x230) : 0x100000U;
    out_info->relocatable_kernel = out_info->boot_protocol >= 0x0205 ? image[0x234] : 0;
    out_info->xloadflags = out_info->boot_protocol >= 0x020c ? read_u16(image + 0x236) : 0;
    out_info->cmdline_size = out_info->boot_protocol >= 0x0206 ? read_u32(image + 0x238) : 0;
    out_info->pref_address = out_info->boot_protocol >= 0x020a ? read_u64(image + 0x258) : 0;
    out_info->init_size = out_info->boot_protocol >= 0x020a ? read_u32(image + 0x260) : 0;
    if (out_info->kernel_alignment == 0 ||
        (out_info->kernel_alignment & (out_info->kernel_alignment - 1U)) != 0) {
        return XBOOT_CORRUPT;
    }
    if (out_info->payload_length == 0 ||
        out_info->payload_offset < setup_bytes ||
        out_info->payload_offset > image_size) {
        return XBOOT_CORRUPT;
    }
    payload_end = (uint64_t)out_info->payload_offset + out_info->payload_length;
    if (payload_end > protected_end || payload_end > UINT32_MAX) return XBOOT_CORRUPT;
    out_info->payload_end = (uint32_t)payload_end;
    out_info->is_bzimage = (uint8_t)((image[0x211] & 1U) != 0);
    if (!out_info->is_bzimage) return XBOOT_PROTOCOL_ERROR;
    return XBOOT_OK;
}
