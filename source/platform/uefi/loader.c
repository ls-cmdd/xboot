#include <xboot/uefi_loader.h>

xboot_status_t xboot_uefi_load_file(EFI_BOOT_SERVICES *boot_services,
                                    EFI_FILE_PROTOCOL *root,
                                    const CHAR16 *path,
                                    xboot_memory_manager *memory,
                                    uint64_t *out_address,
                                    uint64_t *out_page_count,
                                    uint64_t *out_file_size) {
    uint64_t file_size;
    uint64_t page_count;
    uint64_t allocation_size;
    uint64_t address = 0;
    size_t loaded_size = 0;
    xboot_status_t status;
    if (boot_services == NULL || root == NULL || path == NULL || memory == NULL ||
        out_address == NULL || out_page_count == NULL || out_file_size == NULL) return XBOOT_INVALID_ARGUMENT;
    status = xboot_uefi_fs_file_size(boot_services, root, path, &file_size);
    if (status != XBOOT_OK || file_size == 0 || file_size > UINT64_MAX - 4095U) {
        return status == XBOOT_OK ? XBOOT_CORRUPT : status;
    }
    page_count = (file_size + 4095U) / 4096U;
    if (page_count == 0 || page_count > UINT64_MAX / 4096U) return XBOOT_NO_MEMORY;
    allocation_size = page_count * 4096U;
    if (allocation_size > SIZE_MAX) return XBOOT_NO_MEMORY;
    status = xboot_page_alloc(memory, page_count, &address);
    if (status != XBOOT_OK) return status;
    status = xboot_uefi_fs_read_file(boot_services, root, path,
                                     (void *)(uintptr_t)address,
                                     (size_t)allocation_size, &loaded_size);
    if (status != XBOOT_OK || loaded_size != file_size) {
        (void)xboot_page_free(memory, address, page_count);
        return status == XBOOT_OK ? XBOOT_IO_ERROR : status;
    }
    *out_address = address;
    *out_page_count = page_count;
    *out_file_size = file_size;
    return XBOOT_OK;
}
