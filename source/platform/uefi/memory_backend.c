#include <xboot/memory.h>
#include <xboot/uefi.h>

static xboot_status_t map_uefi_status(EFI_STATUS status) {
    if (status == EFI_SUCCESS) {
        return XBOOT_OK;
    }
    if (status == EFI_OUT_OF_RESOURCES) {
        return XBOOT_NO_MEMORY;
    }
    return XBOOT_HARDWARE_ERROR;
}

xboot_status_t xboot_uefi_page_allocate(void *context,
                                        uint64_t page_count,
                                        uint64_t *out_address) {
    EFI_BOOT_SERVICES *boot_services = (EFI_BOOT_SERVICES *)context;
    EFI_PHYSICAL_ADDRESS address = 0;
    EFI_STATUS status;
    if (boot_services == NULL || out_address == NULL || page_count == 0 ||
        page_count > SIZE_MAX) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = boot_services->AllocatePages(EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA,
                                          (UINTN)page_count, &address);
    if (status != EFI_SUCCESS) {
        return map_uefi_status(status);
    }
    *out_address = address;
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_page_allocate_below_4g(void *context,
                                                uint64_t page_count,
                                                uint64_t *out_address) {
    EFI_BOOT_SERVICES *boot_services = (EFI_BOOT_SERVICES *)context;
    EFI_PHYSICAL_ADDRESS address = XBOOT_DIRECT_MAX_PHYSICAL_ADDRESS;
    EFI_STATUS status;
    if (boot_services == NULL || out_address == NULL || page_count == 0 ||
        page_count > SIZE_MAX || boot_services->AllocatePages == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = boot_services->AllocatePages(EFI_ALLOCATE_MAX_ADDRESS, EFI_LOADER_DATA,
                                          (UINTN)page_count, &address);
    if (status != EFI_SUCCESS) return map_uefi_status(status);
    if (address > XBOOT_DIRECT_MAX_PHYSICAL_ADDRESS ||
        page_count > (XBOOT_DIRECT_MAX_PHYSICAL_ADDRESS - address + 1ULL) / 4096ULL) {
        (void)boot_services->FreePages(address, (UINTN)page_count);
        return XBOOT_UNSUPPORTED;
    }
    *out_address = address;
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_page_free(void *context,
                                    uint64_t address,
                                    uint64_t page_count) {
    EFI_BOOT_SERVICES *boot_services = (EFI_BOOT_SERVICES *)context;
    EFI_STATUS status;
    if (boot_services == NULL || address == 0 || page_count == 0 ||
        page_count > SIZE_MAX) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = boot_services->FreePages((EFI_PHYSICAL_ADDRESS)address,
                                      (UINTN)page_count);
    return map_uefi_status(status);
}
