#include <xboot/status.h>
#include <xboot/uefi.h>

xboot_status_t xboot_uefi_exit_boot_services(EFI_HANDLE image_handle,
                                              EFI_BOOT_SERVICES *boot_services,
                                              void *memory_map,
                                              UINTN memory_map_capacity,
                                              UINTN descriptor_size,
                                              UINTN *out_map_key) {
    UINTN map_size;
    UINTN map_key;
    UINTN returned_descriptor_size;
    uint32_t descriptor_version;
    EFI_STATUS status;

    if (image_handle == NULL || boot_services == NULL || memory_map == NULL ||
        memory_map_capacity == 0 || descriptor_size == 0 || out_map_key == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (unsigned int attempt = 0; attempt < 2; attempt++) {
        map_size = memory_map_capacity;
        returned_descriptor_size = descriptor_size;
        descriptor_version = EFI_MEMORY_DESCRIPTOR_VERSION;
        status = boot_services->GetMemoryMap(&map_size, memory_map, &map_key,
                                             &returned_descriptor_size,
                                             &descriptor_version);
        if (status != EFI_SUCCESS) {
            return status == EFI_BUFFER_TOO_SMALL ? XBOOT_NO_MEMORY : XBOOT_IO_ERROR;
        }
        status = boot_services->ExitBootServices(image_handle, map_key);
        if (status == EFI_SUCCESS) {
            *out_map_key = map_key;
            return XBOOT_OK;
        }
        if (status != EFI_INVALID_PARAMETER) {
            return XBOOT_PROTOCOL_ERROR;
        }
    }
    return XBOOT_PROTOCOL_ERROR;
}
