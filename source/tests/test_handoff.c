#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xboot/handoff.h>

static int allocations;

static EFI_STATUS EFIAPI fake_allocate_pool(uint32_t type, UINTN size, void **buffer) {
    (void)type;
    if (buffer == NULL || size == 0) return EFI_INVALID_PARAMETER;
    *buffer = calloc(1, (size_t)size);
    if (*buffer == NULL) return EFI_OUT_OF_RESOURCES;
    allocations++;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_free_pool(void *buffer) {
    if (buffer == NULL) return EFI_INVALID_PARAMETER;
    free(buffer);
    allocations--;
    return EFI_SUCCESS;
}

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    xboot_uefi_handoff handoff = {0};
    uint8_t *old_map;
    void *old_address;
    services.AllocatePool = fake_allocate_pool;
    services.FreePool = fake_free_pool;
    old_map = calloc(1, 64);
    assert(old_map != NULL);
    allocations = 1;
    for (size_t i = 0; i < 64; i++) old_map[i] = (uint8_t)i;
    old_address = old_map;
    handoff.uefi_memory_map = old_map;
    handoff.uefi_memory_map_capacity = 64;
    handoff.memory_descriptor_size = 16;
    assert(xboot_uefi_expand_memory_map(&services, &handoff, 4) == XBOOT_OK);
    assert(handoff.uefi_memory_map != old_address);
    assert(handoff.uefi_memory_map_capacity == 128);
    for (size_t i = 0; i < 64; i++) assert(((uint8_t *)handoff.uefi_memory_map)[i] == (uint8_t)i);
    assert(xboot_uefi_expand_memory_map(&services, &handoff, 0) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_uefi_expand_memory_map(&services, &handoff, UINT64_MAX) == XBOOT_NO_MEMORY);
    assert(allocations == 1);
    assert(services.FreePool(handoff.uefi_memory_map) == EFI_SUCCESS);
    assert(allocations == 0);
    puts("UEFI handoff map expansion host tests: PASS");
    return 0;
}
