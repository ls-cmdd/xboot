#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xboot/uefi_loader.h>

extern xboot_status_t xboot_uefi_page_allocate(void *context, uint64_t page_count, uint64_t *out_address);
extern xboot_status_t xboot_uefi_page_allocate_below_4g(void *context, uint64_t page_count, uint64_t *out_address);
extern xboot_status_t xboot_uefi_page_free(void *context, uint64_t address, uint64_t page_count);

static EFI_FILE_PROTOCOL root;
static EFI_FILE_PROTOCOL file;
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL filesystem;
static uint8_t contents[5000];
static void *allocated;
static EFI_PHYSICAL_ADDRESS allocated_address;
static uint32_t allocated_type;

static EFI_STATUS EFIAPI get_info(EFI_FILE_PROTOCOL *self, EFI_GUID *guid, UINTN *size, void *buffer) {
    (void)self; (void)guid;
    if (size == NULL) return EFI_INVALID_PARAMETER;
    if (buffer == NULL) { *size = sizeof(EFI_FILE_INFO); return EFI_BUFFER_TOO_SMALL; }
    if (*size < sizeof(EFI_FILE_INFO)) return EFI_BUFFER_TOO_SMALL;
    ((EFI_FILE_INFO *)buffer)->FileSize = sizeof(contents);
    ((EFI_FILE_INFO *)buffer)->Attribute = 0;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI read_file(EFI_FILE_PROTOCOL *self, UINTN *size, void *buffer) {
    (void)self;
    if (size == NULL || buffer == NULL || *size < sizeof(contents)) return EFI_INVALID_PARAMETER;
    memcpy(buffer, contents, sizeof(contents));
    *size = sizeof(contents);
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI close_file(EFI_FILE_PROTOCOL *self) { (void)self; return EFI_SUCCESS; }
static EFI_STATUS EFIAPI open_file(EFI_FILE_PROTOCOL *self, EFI_FILE_PROTOCOL **out, CHAR16 *path, uint64_t mode, uint64_t attr) {
    (void)self; (void)path; (void)mode; (void)attr; *out = &file; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI open_volume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *self, EFI_FILE_PROTOCOL **out) {
    (void)self; *out = &root; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI handle_protocol(EFI_HANDLE handle, EFI_GUID *guid, void **out) {
    (void)handle; (void)guid; *out = &filesystem; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI alloc_pool(uint32_t type, UINTN size, void **out) {
    (void)type; *out = malloc((size_t)size); return *out == NULL ? EFI_OUT_OF_RESOURCES : EFI_SUCCESS;
}
static EFI_STATUS EFIAPI free_pool(void *buffer) { free(buffer); return EFI_SUCCESS; }
static EFI_STATUS EFIAPI alloc_pages(uint32_t type, uint32_t memory_type, UINTN pages, EFI_PHYSICAL_ADDRESS *address) {
    (void)memory_type;
    allocated_type = type;
    allocated = aligned_alloc(4096U, (size_t)pages * 4096U);
    if (allocated == NULL) return EFI_OUT_OF_RESOURCES;
    allocated_address = type == EFI_ALLOCATE_MAX_ADDRESS ? 0x200000ULL : (EFI_PHYSICAL_ADDRESS)(uintptr_t)allocated;
    *address = allocated_address;
    return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI free_pages(EFI_PHYSICAL_ADDRESS address, UINTN pages) {
    (void)pages; assert(address == allocated_address); free(allocated); allocated = NULL; allocated_address = 0; return EFI_SUCCESS;
}

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    xboot_page_backend page_backend;
    xboot_memory_manager memory;
    EFI_FILE_PROTOCOL *opened_root = NULL;
    CHAR16 path[] = {'k','e','r','n','e','l',0};
    uint64_t address = 0, pages = 0, file_size = 0;
    for (size_t i = 0; i < sizeof(contents); i++) contents[i] = (uint8_t)i;
    services.HandleProtocol = handle_protocol;
    services.AllocatePool = alloc_pool;
    services.FreePool = free_pool;
    services.AllocatePages = alloc_pages;
    services.FreePages = free_pages;
    filesystem.OpenVolume = open_volume;
    root.Open = open_file;
    root.Close = close_file;
    file.Close = close_file;
    file.Read = read_file;
    file.GetInfo = get_info;
    page_backend.context = &services;
    page_backend.allocate_pages = xboot_uefi_page_allocate;
    page_backend.free_pages = xboot_uefi_page_free;
    assert(xboot_memory_manager_init(&memory, page_backend) == XBOOT_OK);
    assert(xboot_uefi_fs_open(&services, (EFI_HANDLE)&filesystem, &opened_root) == XBOOT_OK);
    assert(xboot_uefi_load_file(&services, opened_root, path, &memory, &address, &pages, &file_size) == XBOOT_OK);
    assert(file_size == sizeof(contents) && pages == 2 && allocated != NULL);
    assert(((uint8_t *)(uintptr_t)address)[4999] == (uint8_t)4999);
    assert(xboot_page_free(&memory, address, pages) == XBOOT_OK);
    assert(allocated == NULL);
    assert(xboot_uefi_page_allocate_below_4g(&services, 2, &address) == XBOOT_OK);
    assert(allocated_type == EFI_ALLOCATE_MAX_ADDRESS && address == 0x200000ULL);
    assert(xboot_uefi_page_free(&services, address, 2) == XBOOT_OK);
    puts("UEFI page-backed loader host tests: PASS");
    return 0;
}
