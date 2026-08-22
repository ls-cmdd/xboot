#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xboot/uefi_fs.h>

static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL filesystem;
static EFI_FILE_PROTOCOL root;
static EFI_FILE_PROTOCOL file;
static const uint8_t contents[] = {'k','e','r','n','e','l'};

static EFI_STATUS EFIAPI fake_get_info(EFI_FILE_PROTOCOL *self, EFI_GUID *guid,
                                       UINTN *size, void *buffer) {
    (void)self; (void)guid;
    if (size == NULL) return EFI_INVALID_PARAMETER;
    if (buffer == NULL) { *size = sizeof(EFI_FILE_INFO); return EFI_BUFFER_TOO_SMALL; }
    if (*size < sizeof(EFI_FILE_INFO)) return EFI_BUFFER_TOO_SMALL;
    ((EFI_FILE_INFO *)buffer)->Size = sizeof(EFI_FILE_INFO);
    ((EFI_FILE_INFO *)buffer)->FileSize = sizeof(contents);
    ((EFI_FILE_INFO *)buffer)->Attribute = 0;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_read(EFI_FILE_PROTOCOL *self, UINTN *size, void *buffer) {
    (void)self;
    if (size == NULL || buffer == NULL || *size < sizeof(contents)) return EFI_INVALID_PARAMETER;
    for (size_t i = 0; i < sizeof(contents); i++) ((uint8_t *)buffer)[i] = contents[i];
    *size = sizeof(contents);
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_close(EFI_FILE_PROTOCOL *self) { (void)self; return EFI_SUCCESS; }

static EFI_STATUS EFIAPI fake_open(EFI_FILE_PROTOCOL *self, EFI_FILE_PROTOCOL **out,
                                   CHAR16 *path, uint64_t mode, uint64_t attributes) {
    (void)self; (void)path; (void)mode; (void)attributes;
    if (out == NULL) return EFI_INVALID_PARAMETER;
    *out = &file;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_open_volume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *self,
                                          EFI_FILE_PROTOCOL **out) {
    (void)self;
    if (out == NULL) return EFI_INVALID_PARAMETER;
    *out = &root;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_handle_protocol(EFI_HANDLE handle, EFI_GUID *guid, void **interface) {
    (void)handle; (void)guid;
    if (interface == NULL) return EFI_INVALID_PARAMETER;
    *interface = &filesystem;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_allocate_pool(uint32_t type, UINTN size, void **buffer) {
    (void)type;
    if (buffer == NULL) return EFI_INVALID_PARAMETER;
    *buffer = malloc((size_t)size);
    return *buffer == NULL ? EFI_OUT_OF_RESOURCES : EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_free_pool(void *buffer) { free(buffer); return EFI_SUCCESS; }

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    CHAR16 path[] = {'k','e','r','n','e','l',0};
    uint8_t buffer[sizeof(contents)] = {0};
    size_t size = 0;
    services.HandleProtocol = fake_handle_protocol;
    services.AllocatePool = fake_allocate_pool;
    services.FreePool = fake_free_pool;
    filesystem.OpenVolume = fake_open_volume;
    root.Open = fake_open;
    root.Close = fake_close;
    file.Close = fake_close;
    file.Read = fake_read;
    file.GetInfo = fake_get_info;
    EFI_FILE_PROTOCOL *opened_root = NULL;
    assert(xboot_uefi_fs_open(&services, (EFI_HANDLE)&filesystem, &opened_root) == XBOOT_OK);
    assert(xboot_uefi_fs_read_file(&services, opened_root, path, buffer, sizeof(buffer), &size) == XBOOT_OK);
    assert(size == sizeof(contents) && memcmp(buffer, contents, sizeof(contents)) == 0);
    assert(xboot_uefi_fs_read_file(&services, opened_root, path, buffer, sizeof(buffer) - 1, &size) == XBOOT_BUFFER_TOO_SMALL);
    puts("UEFI filesystem adapter host tests: PASS");
    return 0;
}
