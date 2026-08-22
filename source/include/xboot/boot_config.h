#ifndef XBOOT_BOOT_CONFIG_H
#define XBOOT_BOOT_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/sha256.h>

#define XBOOT_CONFIG_STRING_MAX 256U

typedef struct {
    char title[XBOOT_CONFIG_STRING_MAX];
    char linux_path[XBOOT_CONFIG_STRING_MAX];
    char initrd_path[XBOOT_CONFIG_STRING_MAX];
    char options[XBOOT_CONFIG_STRING_MAX];
    char kernel_sha256[XBOOT_SHA256_HEX_SIZE];
    char initrd_sha256[XBOOT_SHA256_HEX_SIZE];
    uint32_t timeout_seconds;
    uint8_t has_linux;
    uint8_t has_kernel_sha256;
    uint8_t has_initrd_sha256;
    uint8_t measure_required;
    uint8_t secure_boot_required;
} xboot_boot_entry;

xboot_status_t xboot_boot_config_parse(const uint8_t *text,
                                       size_t text_size,
                                       xboot_boot_entry *out_entry);

#endif
