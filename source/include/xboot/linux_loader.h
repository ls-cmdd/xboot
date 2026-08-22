#ifndef XBOOT_LINUX_LOADER_H
#define XBOOT_LINUX_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/linux_image.h>
#include <xboot/memory.h>
#include <xboot/handoff.h>

#define XBOOT_LINUX_BOOT_PARAMS_SIZE 4096U
#define XBOOT_LINUX_PAGE_SIZE 4096U
#define XBOOT_LINUX_MAX_CMDLINE 4096U

typedef struct {
    uint64_t setup_address;
    uint64_t kernel_address;
    uint64_t kernel_allocation_address;
    uint64_t kernel_allocation_pages;
    uint64_t initrd_address;
    uint64_t initrd_allocation_address;
    uint64_t initrd_allocation_pages;
    uint64_t cmdline_address;
    uint64_t boot_params_address;
    uint64_t boot_params_pages;
    uint64_t direct_page_table_address;
    uint64_t direct_page_table_pages;
    uint64_t direct_stack_address;
    uint64_t direct_stack_pages;
    uint64_t kernel_size;
    uint64_t initrd_size;
    uint32_t e820_entries;
    uint8_t boot_params_built;
    uint8_t has_initrd;
    uint8_t prepared;
} xboot_linux_boot_plan;

xboot_status_t xboot_linux_load_plan(const uint8_t *image,
                                     size_t image_size,
                                     const uint8_t *initrd,
                                     size_t initrd_size,
                                     const char *cmdline,
                                     xboot_memory_manager *memory,
                                     xboot_linux_boot_plan *out_plan);

xboot_status_t xboot_linux_build_boot_params(const uint8_t *image,
                                              size_t image_size,
                                              const xboot_uefi_handoff *handoff,
                                              xboot_linux_boot_plan *plan);

xboot_status_t xboot_linux_release_plan(xboot_memory_manager *memory,
                                        xboot_linux_boot_plan *plan);

xboot_status_t xboot_linux_direct_start(EFI_HANDLE image_handle,
                                         EFI_BOOT_SERVICES *boot_services,
                                         xboot_uefi_handoff *handoff,
                                         xboot_memory_manager *memory,
                                         xboot_linux_boot_plan *plan);

#endif
