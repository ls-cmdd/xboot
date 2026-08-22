#include <xboot/uefi_linux.h>

#define EFI_DEVICE_PATH_TYPE_MEDIA 4U
#define EFI_DEVICE_PATH_SUBTYPE_FILE 4U
#define EFI_DEVICE_PATH_TYPE_END 0x7fU
#define EFI_DEVICE_PATH_SUBTYPE_END 0xffU
#define EFI_NOT_FOUND_STATUS 0x800000000000000eULL
#define EFI_SECURITY_VIOLATION_STATUS 0x8000000000000026ULL
#define EFI_UNSUPPORTED_STATUS 0x8000000000000003ULL
#define XBOOT_MAX_EFI_FILE_PATH_CHARS 240U
#define XBOOT_MAX_DEVICE_PATH_BYTES 1024U

static xboot_status_t map_status(EFI_STATUS status) {
    if (status == EFI_SUCCESS) return XBOOT_OK;
    if (status == EFI_OUT_OF_RESOURCES || status == EFI_BUFFER_TOO_SMALL) return XBOOT_NO_MEMORY;
    if (status == EFI_INVALID_PARAMETER) return XBOOT_INVALID_ARGUMENT;
    if (status == EFI_NOT_FOUND_STATUS) return XBOOT_NOT_FOUND;
    if (status == EFI_SECURITY_VIOLATION_STATUS) return XBOOT_SECURITY_ERROR;
    if (status == EFI_UNSUPPORTED_STATUS) return XBOOT_UNSUPPORTED;
    return XBOOT_PROTOCOL_ERROR;
}

static uint16_t read16(const uint8_t *address) {
    return (uint16_t)address[0] | ((uint16_t)address[1] << 8);
}

static void put16(uint8_t *address, uint16_t value) {
    address[0] = (uint8_t)value;
    address[1] = (uint8_t)(value >> 8);
}

static uint16_t path_length(const CHAR16 *path, int *valid) {
    uint16_t length = 0;
    *valid = 0;
    if (path == NULL) return 0;
    while (path[length] != 0) {
        if (length >= XBOOT_MAX_EFI_FILE_PATH_CHARS) return 0;
        length++;
    }
    *valid = 1;
    return length;
}

static size_t device_path_end_size(const EFI_DEVICE_PATH_PROTOCOL *path) {
    const uint8_t *bytes = (const uint8_t *)path;
    size_t offset = 0;
    if (path == NULL) return 0;
    while (offset + 4U <= XBOOT_MAX_DEVICE_PATH_BYTES) {
        uint16_t length = read16(bytes + offset + 2U);
        if (length < 4U || offset + length > XBOOT_MAX_DEVICE_PATH_BYTES) return 0;
        if (bytes[offset] == EFI_DEVICE_PATH_TYPE_END &&
            bytes[offset + 1U] == EFI_DEVICE_PATH_SUBTYPE_END) {
            return offset + length;
        }
        offset += length;
    }
    return 0;
}

static xboot_status_t build_linux_device_path(EFI_BOOT_SERVICES *boot_services,
                                              EFI_HANDLE parent_image,
                                              const CHAR16 *file_path,
                                              uint8_t *new_device_path,
                                              size_t new_device_path_size,
                                              EFI_STATUS *out_efi_status,
                                              uint32_t *out_stage) {
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID device_path_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *parent_loaded_image = NULL;
    EFI_DEVICE_PATH_PROTOCOL *device_path = NULL;
    EFI_STATUS efi_status;
    uint16_t chars;
    uint16_t file_node_length;
    size_t base_size;
    size_t base_prefix;
    size_t total_size;
    int valid;
    xboot_status_t status;
    if (out_efi_status != NULL) *out_efi_status = EFI_SUCCESS;
    if (out_stage != NULL) *out_stage = 0;
    if (boot_services == NULL || parent_image == NULL || file_path == NULL ||
        new_device_path == NULL || new_device_path_size < 4U ||
        boot_services->HandleProtocol == NULL) return XBOOT_INVALID_ARGUMENT;
    if (out_stage != NULL) *out_stage = 1;
    efi_status = boot_services->HandleProtocol(parent_image, &loaded_image_guid,
                                               (void **)&parent_loaded_image);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    status = map_status(efi_status);
    if (status != XBOOT_OK || parent_loaded_image == NULL ||
        parent_loaded_image->DeviceHandle == NULL) return status == XBOOT_OK ? XBOOT_NOT_FOUND : status;
    if (out_stage != NULL) *out_stage = 2;
    efi_status = boot_services->HandleProtocol(parent_loaded_image->DeviceHandle,
                                               &device_path_guid, (void **)&device_path);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    status = map_status(efi_status);
    if (status != XBOOT_OK || device_path == NULL) return status;
    base_size = device_path_end_size(device_path);
    if (base_size < 4U) return XBOOT_PROTOCOL_ERROR;
    base_prefix = base_size - 4U;
    chars = path_length(file_path, &valid);
    if (!valid) return XBOOT_INVALID_ARGUMENT;
    file_node_length = (uint16_t)(4U + (chars + 1U) * sizeof(CHAR16));
    total_size = base_prefix + file_node_length + 4U;
    if (total_size > new_device_path_size) return XBOOT_BUFFER_TOO_SMALL;
    for (size_t i = 0; i < base_prefix; i++) new_device_path[i] = ((const uint8_t *)device_path)[i];
    new_device_path[base_prefix] = EFI_DEVICE_PATH_TYPE_MEDIA;
    new_device_path[base_prefix + 1U] = EFI_DEVICE_PATH_SUBTYPE_FILE;
    put16(new_device_path + base_prefix + 2U, file_node_length);
    for (uint16_t i = 0; i <= chars; i++) {
        put16(new_device_path + base_prefix + 4U + i * 2U, file_path[i]);
    }
    new_device_path[base_prefix + file_node_length] = EFI_DEVICE_PATH_TYPE_END;
    new_device_path[base_prefix + file_node_length + 1U] = EFI_DEVICE_PATH_SUBTYPE_END;
    put16(new_device_path + base_prefix + file_node_length + 2U, 4U);
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_start_linux_file(EFI_BOOT_SERVICES *boot_services,
                                           EFI_HANDLE parent_image,
                                           const CHAR16 *file_path,
                                           const CHAR16 *load_options,
                                           UINTN load_options_size,
                                           EFI_HANDLE *out_image,
                                           EFI_STATUS *out_efi_status,
                                           uint32_t *out_stage) {
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_HANDLE linux_image = NULL;
    EFI_STATUS efi_status = EFI_SUCCESS;
    UINTN exit_data_size = 0;
    CHAR16 *exit_data = NULL;
    uint8_t new_device_path[XBOOT_MAX_DEVICE_PATH_BYTES];
    xboot_status_t status;
    if (out_efi_status != NULL) *out_efi_status = EFI_SUCCESS;
    if (out_stage != NULL) *out_stage = 0;
    if (boot_services == NULL || parent_image == NULL || load_options == NULL ||
        load_options_size == 0 || out_image == NULL || file_path == NULL ||
        load_options_size > UINT32_MAX || boot_services->LoadImage == NULL ||
        boot_services->StartImage == NULL || boot_services->HandleProtocol == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = build_linux_device_path(boot_services, parent_image, file_path,
                                     new_device_path, sizeof(new_device_path),
                                     out_efi_status, out_stage);
    if (status != XBOOT_OK) return status;
    *out_image = NULL;
    if (out_stage != NULL) *out_stage = 3;
    efi_status = boot_services->LoadImage(0, parent_image, new_device_path, NULL, 0, &linux_image);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    status = map_status(efi_status);
    if (status != XBOOT_OK || linux_image == NULL) return status;
    if (out_stage != NULL) *out_stage = 4;
    efi_status = boot_services->HandleProtocol(linux_image, &loaded_image_guid,
                                               (void **)&loaded_image);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    status = map_status(efi_status);
    if (status != XBOOT_OK || loaded_image == NULL) {
        if (boot_services->UnloadImage != NULL && linux_image != NULL) {
            (void)boot_services->UnloadImage(linux_image);
        }
        return status;
    }
    loaded_image->LoadOptions = (void *)load_options;
    loaded_image->LoadOptionsSize = (uint32_t)load_options_size;
    *out_image = linux_image;
    if (out_stage != NULL) *out_stage = 5;
    efi_status = boot_services->StartImage(linux_image, &exit_data_size, &exit_data);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    if (efi_status != EFI_SUCCESS && boot_services->FreePool != NULL && exit_data != NULL) {
        (void)boot_services->FreePool(exit_data);
    }
    return map_status(efi_status);
}

xboot_status_t xboot_uefi_authenticate_linux_file(EFI_BOOT_SERVICES *boot_services,
                                                  EFI_HANDLE parent_image,
                                                  const CHAR16 *file_path,
                                                  EFI_STATUS *out_efi_status,
                                                  uint32_t *out_stage) {
    EFI_HANDLE linux_image = NULL;
    EFI_STATUS efi_status = EFI_SUCCESS;
    uint8_t new_device_path[XBOOT_MAX_DEVICE_PATH_BYTES];
    xboot_status_t status;
    if (out_efi_status != NULL) *out_efi_status = EFI_SUCCESS;
    if (out_stage != NULL) *out_stage = 0;
    status = build_linux_device_path(boot_services, parent_image, file_path,
                                     new_device_path, sizeof(new_device_path),
                                     out_efi_status, out_stage);
    if (status != XBOOT_OK || boot_services == NULL || boot_services->LoadImage == NULL) {
        return status != XBOOT_OK ? status : XBOOT_INVALID_ARGUMENT;
    }
    if (out_stage != NULL) *out_stage = 3;
    efi_status = boot_services->LoadImage(0, parent_image, new_device_path, NULL, 0, &linux_image);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    status = map_status(efi_status);
    if (status != XBOOT_OK || linux_image == NULL) return status;
    if (boot_services->UnloadImage == NULL) return XBOOT_UNSUPPORTED;
    if (out_stage != NULL) *out_stage = 4;
    efi_status = boot_services->UnloadImage(linux_image);
    if (out_efi_status != NULL) *out_efi_status = efi_status;
    return map_status(efi_status);
}
