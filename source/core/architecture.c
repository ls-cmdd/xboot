#include <stddef.h>
#include <xboot/architecture.h>

static void clear_info(xboot_architecture_info *info) {
    for (size_t i = 0; i < sizeof(*info); i++) ((uint8_t *)info)[i] = 0;
}

xboot_status_t xboot_architecture_current(xboot_architecture_info *out_info) {
    if (out_info == NULL) return XBOOT_INVALID_ARGUMENT;
    clear_info(out_info);
#if defined(__x86_64__)
    out_info->architecture = XBOOT_ARCH_X86_64;
    out_info->platform = XBOOT_PLATFORM_UEFI;
    out_info->compiled_for_target = 1;
    out_info->implementation_available = 1;
    return XBOOT_OK;
#elif defined(__aarch64__)
    out_info->architecture = XBOOT_ARCH_ARM64;
    out_info->platform = XBOOT_PLATFORM_UNKNOWN;
    out_info->compiled_for_target = 1;
    out_info->implementation_available = 0;
    return XBOOT_UNSUPPORTED;
#elif defined(__riscv) && (__riscv_xlen == 64)
    out_info->architecture = XBOOT_ARCH_RISCV64;
    out_info->platform = XBOOT_PLATFORM_UNKNOWN;
    out_info->compiled_for_target = 1;
    out_info->implementation_available = 0;
    return XBOOT_UNSUPPORTED;
#else
    out_info->architecture = XBOOT_ARCH_UNKNOWN;
    out_info->platform = XBOOT_PLATFORM_UNKNOWN;
    return XBOOT_UNSUPPORTED;
#endif
}

xboot_status_t xboot_architecture_query(xboot_architecture architecture,
                                        xboot_platform platform,
                                        xboot_architecture_info *out_info) {
    if (out_info == NULL) return XBOOT_INVALID_ARGUMENT;
    clear_info(out_info);
    out_info->architecture = architecture;
    out_info->platform = platform;
    if (architecture == XBOOT_ARCH_X86_64 && platform == XBOOT_PLATFORM_UEFI) {
        out_info->compiled_for_target = 1;
        out_info->implementation_available = 1;
        return XBOOT_OK;
    }
    return XBOOT_UNSUPPORTED;
}
