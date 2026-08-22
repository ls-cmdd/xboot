#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/uefi_linux.h>

#define XBOOT_TEST_LONG_PATH_CHARS 242U

static int start_called;
static int unload_called;
static EFI_STATUS load_status = EFI_SUCCESS;
static EFI_HANDLE parent_handle = (EFI_HANDLE)(uintptr_t)0x1000;
static EFI_HANDLE device_handle = (EFI_HANDLE)(uintptr_t)0x2000;
static EFI_HANDLE child_handle = (EFI_HANDLE)(uintptr_t)0x3000;
static EFI_LOADED_IMAGE_PROTOCOL parent_image;
static EFI_LOADED_IMAGE_PROTOCOL child_image;
static uint8_t base_path[] = {1, 1, 5, 0, 0, 0x7f, 0xff, 4, 0};

static int guid_data1(const EFI_GUID *guid, uint32_t value) {
    return guid != NULL && guid->Data1 == value;
}

static EFI_STATUS EFIAPI fake_handle_protocol(EFI_HANDLE handle, EFI_GUID *guid, void **interface) {
    if (interface == NULL) return EFI_INVALID_PARAMETER;
    if (handle == parent_handle && guid_data1(guid, 0x5B1B31A1)) {
        *interface = &parent_image;
        return EFI_SUCCESS;
    }
    if (handle == device_handle && guid_data1(guid, 0x09576E91)) {
        *interface = base_path;
        return EFI_SUCCESS;
    }
    if (handle == child_handle && guid_data1(guid, 0x5B1B31A1)) {
        *interface = &child_image;
        return EFI_SUCCESS;
    }
    return 0x800000000000000eULL;
}

static EFI_STATUS EFIAPI fake_load_image(BOOLEAN boot_policy, EFI_HANDLE parent,
                                         void *path, void *source, UINTN source_size,
                                         EFI_HANDLE *image) {
    const uint8_t *bytes = path;
    (void)boot_policy;
    (void)source;
    (void)source_size;
    assert(parent == parent_handle);
    assert(bytes[5] == 4 && bytes[6] == 4);
    assert(bytes[9] == '\\' && bytes[10] == 0 && bytes[11] == 'E');
    assert(image != NULL);
    *image = child_handle;
    return load_status;
}

static EFI_STATUS EFIAPI fake_unload_image(EFI_HANDLE image) {
    assert(image == child_handle);
    unload_called = 1;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_start_image(EFI_HANDLE image, UINTN *exit_size, CHAR16 **exit_data) {
    assert(image == child_handle);
    assert(child_image.LoadOptions != NULL && child_image.LoadOptionsSize > 0);
    assert(exit_size != NULL && exit_data != NULL);
    start_called = 1;
    return EFI_SUCCESS;
}

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    EFI_HANDLE output = NULL;
    EFI_STATUS efi_status = 0;
    uint32_t stage = 0;
    static const CHAR16 path[] = {'\\','E','F','I','\\','X','B','O','O','T','\\','v','m','l','i','n','u','z','.','e','f','i',0};
    static const CHAR16 options[] = {'c','o','n','s','o','l','e','=','t','t','y','S','0',0};
    parent_image.DeviceHandle = device_handle;
    services.HandleProtocol = fake_handle_protocol;
    services.LoadImage = fake_load_image;
    services.StartImage = fake_start_image;
    services.UnloadImage = fake_unload_image;
    assert(xboot_uefi_start_linux_file(&services, parent_handle, path, options,
                                       sizeof(options), &output, &efi_status, &stage) == XBOOT_OK);
    assert(output == child_handle && efi_status == EFI_SUCCESS && stage == 5 && start_called == 1);
    unload_called = 0;
    assert(xboot_uefi_authenticate_linux_file(&services, parent_handle, path, &efi_status, &stage) == XBOOT_OK);
    assert(efi_status == EFI_SUCCESS && stage == 4 && unload_called == 1);
    load_status = EFI_SECURITY_VIOLATION;
    assert(xboot_uefi_authenticate_linux_file(&services, parent_handle, path, &efi_status, &stage) == XBOOT_SECURITY_ERROR);
    assert(efi_status == EFI_SECURITY_VIOLATION && unload_called == 1);
    load_status = EFI_SUCCESS;
    parent_image.DeviceHandle = NULL;
    assert(xboot_uefi_start_linux_file(&services, parent_handle, path, options,
                                       sizeof(options), &output, &efi_status, &stage) == XBOOT_NOT_FOUND);
    {
        CHAR16 long_path[XBOOT_TEST_LONG_PATH_CHARS];
        for (size_t i = 0; i + 1U < sizeof(long_path) / sizeof(long_path[0]); i++) long_path[i] = 'a';
        long_path[(sizeof(long_path) / sizeof(long_path[0])) - 1U] = 0;
        parent_image.DeviceHandle = device_handle;
        assert(xboot_uefi_start_linux_file(&services, parent_handle, long_path, options,
                                           sizeof(options), &output, &efi_status, &stage) == XBOOT_INVALID_ARGUMENT);
    }
    puts("UEFI Linux PE/COFF starter host tests: PASS");
    return 0;
}
