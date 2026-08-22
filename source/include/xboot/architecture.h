#ifndef XBOOT_ARCHITECTURE_H
#define XBOOT_ARCHITECTURE_H

#include <stdint.h>
#include <xboot/status.h>

typedef enum {
    XBOOT_ARCH_UNKNOWN = 0,
    XBOOT_ARCH_X86_64 = 1,
    XBOOT_ARCH_ARM64 = 2,
    XBOOT_ARCH_RISCV64 = 3,
} xboot_architecture;

typedef enum {
    XBOOT_PLATFORM_UNKNOWN = 0,
    XBOOT_PLATFORM_UEFI = 1,
    XBOOT_PLATFORM_BIOS = 2,
} xboot_platform;

typedef struct {
    xboot_architecture architecture;
    xboot_platform platform;
    uint8_t compiled_for_target;
    uint8_t implementation_available;
} xboot_architecture_info;

xboot_status_t xboot_architecture_current(xboot_architecture_info *out_info);
xboot_status_t xboot_architecture_query(xboot_architecture architecture,
                                        xboot_platform platform,
                                        xboot_architecture_info *out_info);

#endif
