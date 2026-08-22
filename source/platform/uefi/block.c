#include <xboot/uefi_block.h>

static xboot_status_t translate_status(EFI_STATUS status) {
    if (status == EFI_SUCCESS) return XBOOT_OK;
    if (status == EFI_OUT_OF_RESOURCES) return XBOOT_NO_MEMORY;
    if (status == EFI_INVALID_PARAMETER) return XBOOT_INVALID_ARGUMENT;
    return XBOOT_IO_ERROR;
}

static xboot_status_t uefi_read_blocks(void *context, uint32_t media_id,
                                       uint64_t lba, size_t buffer_size,
                                       void *buffer) {
    EFI_BLOCK_IO_PROTOCOL *block_io = context;
    if (block_io == NULL || block_io->Media == NULL || block_io->ReadBlocks == NULL || buffer == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if (media_id != block_io->Media->MediaId) return XBOOT_IO_ERROR;
    return translate_status(block_io->ReadBlocks(block_io, media_id, lba,
                                                 (UINTN)buffer_size, buffer));
}

xboot_status_t xboot_uefi_block_from_device(EFI_BOOT_SERVICES *boot_services,
                                            EFI_HANDLE device_handle,
                                            xboot_block_device *out_device) {
    EFI_GUID guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *block_io = NULL;
    EFI_STATUS status;
    if (boot_services == NULL || boot_services->HandleProtocol == NULL ||
        device_handle == NULL || out_device == NULL) return XBOOT_INVALID_ARGUMENT;
    status = boot_services->HandleProtocol(device_handle, &guid, (void **)&block_io);
    if (status != EFI_SUCCESS || block_io == NULL || block_io->Media == NULL ||
        block_io->ReadBlocks == NULL || block_io->Media->MediaPresent == 0 ||
        block_io->Media->BlockSize == 0) return translate_status(status == EFI_SUCCESS ? EFI_INVALID_PARAMETER : status);
    out_device->context = block_io;
    out_device->read_blocks = uefi_read_blocks;
    out_device->media_id = block_io->Media->MediaId;
    out_device->block_size = block_io->Media->BlockSize;
    out_device->last_block = block_io->Media->LastBlock;
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_block_locate_first(EFI_BOOT_SERVICES *boot_services,
                                             xboot_block_device *out_device,
                                             EFI_HANDLE *out_handle) {
    EFI_GUID guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_HANDLE *handles = NULL;
    UINTN handle_count = 0;
    EFI_STATUS status;
    xboot_status_t result;
    if (boot_services == NULL || boot_services->LocateHandleBuffer == NULL || out_device == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    status = boot_services->LocateHandleBuffer(EFI_LOCATE_BY_PROTOCOL, &guid,
                                               NULL, &handle_count, &handles);
    if (status != EFI_SUCCESS || handle_count == 0 || handles == NULL) return translate_status(status == EFI_SUCCESS ? EFI_INVALID_PARAMETER : status);
    result = xboot_uefi_block_from_device(boot_services, handles[0], out_device);
    if (out_handle != NULL) *out_handle = handles[0];
    if (boot_services->FreePool != NULL) boot_services->FreePool(handles);
    return result;
}
