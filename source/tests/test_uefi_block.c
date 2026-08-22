#include <assert.h>
#include <stdio.h>
#include <xboot/uefi_block.h>

static EFI_BLOCK_IO_PROTOCOL block_io;
static EFI_BLOCK_IO_MEDIA media;
static EFI_HANDLE handles[1];

static EFI_STATUS EFIAPI fake_read(EFI_BLOCK_IO_PROTOCOL *protocol, uint32_t media_id,
                                   EFI_LBA lba, UINTN buffer_size, void *buffer) {
    (void)protocol;
    if (media_id != media.MediaId || buffer == NULL || buffer_size != media.BlockSize) return EFI_INVALID_PARAMETER;
    ((uint8_t *)buffer)[0] = (uint8_t)lba;
    ((uint8_t *)buffer)[1] = 0xA5;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_handle_protocol(EFI_HANDLE handle, EFI_GUID *guid, void **interface) {
    (void)guid;
    if (handle != (EFI_HANDLE)&block_io || interface == NULL) return EFI_INVALID_PARAMETER;
    *interface = &block_io;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_locate(uint32_t search_type, EFI_GUID *guid, void *key,
                                     UINTN *count, EFI_HANDLE **out_handles) {
    (void)search_type; (void)guid; (void)key;
    if (count == NULL || out_handles == NULL) return EFI_INVALID_PARAMETER;
    *count = 1;
    *out_handles = handles;
    return EFI_SUCCESS;
}

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    xboot_block_device device;
    uint8_t buffer[512] = {0};
    EFI_HANDLE handle = NULL;
    media.MediaId = 7;
    media.MediaPresent = 1;
    media.BlockSize = sizeof(buffer);
    media.LastBlock = 99;
    block_io.Media = &media;
    block_io.ReadBlocks = fake_read;
    handles[0] = (EFI_HANDLE)&block_io;
    services.HandleProtocol = fake_handle_protocol;
    assert(xboot_uefi_block_from_device(&services, handles[0], &device) == XBOOT_OK);
    assert(device.media_id == 7 && device.block_size == sizeof(buffer) && device.last_block == 99);
    assert(xboot_block_read(&device, 4, 1, buffer, sizeof(buffer)) == XBOOT_OK);
    assert(buffer[0] == 4 && buffer[1] == 0xA5);
    services.LocateHandleBuffer = fake_locate;
    services.FreePool = NULL;
    assert(xboot_uefi_block_locate_first(&services, &device, &handle) == XBOOT_OK);
    assert(handle == handles[0]);
    puts("UEFI Block I/O adapter host tests: PASS");
    return 0;
}
