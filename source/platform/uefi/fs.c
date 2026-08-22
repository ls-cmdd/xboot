#include <xboot/uefi_fs.h>

static xboot_status_t translate_status(EFI_STATUS status) {
    if (status == EFI_SUCCESS) return XBOOT_OK;
    if (status == EFI_BUFFER_TOO_SMALL) return XBOOT_BUFFER_TOO_SMALL;
    if (status == EFI_OUT_OF_RESOURCES) return XBOOT_NO_MEMORY;
    if (status == EFI_INVALID_PARAMETER) return XBOOT_INVALID_ARGUMENT;
    return XBOOT_IO_ERROR;
}

xboot_status_t xboot_uefi_fs_open(EFI_BOOT_SERVICES *boot_services,
                                  EFI_HANDLE device_handle,
                                  EFI_FILE_PROTOCOL **out_root) {
    EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *filesystem = NULL;
    EFI_STATUS status;
    if (boot_services == NULL || boot_services->HandleProtocol == NULL ||
        device_handle == NULL || out_root == NULL) return XBOOT_INVALID_ARGUMENT;
    *out_root = NULL;
    status = boot_services->HandleProtocol(device_handle, &guid, (void **)&filesystem);
    if (status != EFI_SUCCESS || filesystem == NULL || filesystem->OpenVolume == NULL) {
        return translate_status(status == EFI_SUCCESS ? EFI_INVALID_PARAMETER : status);
    }
    status = filesystem->OpenVolume(filesystem, out_root);
    if (status != EFI_SUCCESS || *out_root == NULL) return translate_status(status);
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_fs_file_size(EFI_BOOT_SERVICES *boot_services,
                                       EFI_FILE_PROTOCOL *root,
                                       const CHAR16 *path,
                                       uint64_t *out_size) {
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_GUID info_guid = EFI_FILE_INFO_ID;
    EFI_FILE_INFO *info = NULL;
    UINTN info_size = 0;
    EFI_STATUS status;
    if (boot_services == NULL || root == NULL || path == NULL || out_size == NULL ||
        root->Open == NULL || root->Close == NULL || boot_services->AllocatePool == NULL ||
        boot_services->FreePool == NULL) return XBOOT_INVALID_ARGUMENT;
    status = root->Open(root, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || file == NULL || file->GetInfo == NULL) {
        if (file != NULL && file->Close != NULL) (void)file->Close(file);
        return translate_status(status == EFI_SUCCESS ? EFI_INVALID_PARAMETER : status);
    }
    status = file->GetInfo(file, &info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL || info_size < sizeof(EFI_FILE_INFO)) {
        (void)file->Close(file);
        return translate_status(status);
    }
    status = boot_services->AllocatePool(EFI_LOADER_DATA, info_size, (void **)&info);
    if (status != EFI_SUCCESS || info == NULL) {
        (void)file->Close(file);
        return translate_status(status);
    }
    status = file->GetInfo(file, &info_guid, &info_size, info);
    if (status != EFI_SUCCESS || (info->Attribute & EFI_FILE_DIRECTORY) != 0) {
        xboot_status_t result = status == EFI_SUCCESS ? XBOOT_INVALID_ARGUMENT : translate_status(status);
        (void)boot_services->FreePool(info);
        (void)file->Close(file);
        return result;
    }
    *out_size = info->FileSize;
    (void)boot_services->FreePool(info);
    (void)file->Close(file);
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_fs_read_file(EFI_BOOT_SERVICES *boot_services,
                                       EFI_FILE_PROTOCOL *root,
                                       const CHAR16 *path, void *buffer,
                                       size_t buffer_size, size_t *out_size) {
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_GUID info_guid = EFI_FILE_INFO_ID;
    EFI_FILE_INFO *info = NULL;
    UINTN info_size = 0;
    UINTN read_size;
    EFI_STATUS status;
    xboot_status_t result;
    if (boot_services == NULL || root == NULL || path == NULL || root->Open == NULL || root->Close == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = root->Open(root, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) return translate_status(status);
    if (file == NULL || file->GetInfo == NULL || file->Read == NULL) {
        if (file != NULL) (void)file->Close(file);
        return XBOOT_UNSUPPORTED;
    }
    status = file->GetInfo(file, &info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL || info_size < sizeof(EFI_FILE_INFO)) {
        (void)file->Close(file);
        return translate_status(status);
    }
    if (boot_services->AllocatePool == NULL || boot_services->FreePool == NULL) {
        (void)file->Close(file);
        return XBOOT_UNSUPPORTED;
    }
    status = boot_services->AllocatePool(EFI_LOADER_DATA, info_size, (void **)&info);
    if (status != EFI_SUCCESS || info == NULL) {
        (void)file->Close(file);
        return translate_status(status);
    }
    status = file->GetInfo(file, &info_guid, &info_size, info);
    if (status != EFI_SUCCESS || (info->Attribute & EFI_FILE_DIRECTORY) != 0 || info->FileSize > buffer_size) {
        result = (status != EFI_SUCCESS) ? translate_status(status) : XBOOT_BUFFER_TOO_SMALL;
        (void)boot_services->FreePool(info);
        (void)file->Close(file);
        return result;
    }
    read_size = (UINTN)info->FileSize;
    if (read_size != 0 && buffer == NULL) {
        (void)boot_services->FreePool(info);
        (void)file->Close(file);
        return XBOOT_INVALID_ARGUMENT;
    }
    status = file->Read(file, &read_size, buffer);
    result = (status == EFI_SUCCESS && read_size == (UINTN)info->FileSize) ? XBOOT_OK : translate_status(status);
    if (out_size != NULL) *out_size = (size_t)read_size;
    (void)boot_services->FreePool(info);
    (void)file->Close(file);
    return result;
}
