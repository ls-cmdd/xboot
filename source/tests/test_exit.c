#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/status.h>
#include <xboot/uefi.h>

extern xboot_status_t xboot_uefi_exit_boot_services(EFI_HANDLE image_handle,
                                                     EFI_BOOT_SERVICES *boot_services,
                                                     void *memory_map,
                                                     UINTN memory_map_capacity,
                                                     UINTN descriptor_size,
                                                     UINTN *out_map_key);

typedef struct {
    unsigned int map_calls;
    unsigned int exit_calls;
} exit_state;

static exit_state *global_state;

static EFI_STATUS EFIAPI fake_get_memory_map(UINTN *size, void *map, UINTN *key,
                                             UINTN *descriptor_size, uint32_t *version) {
    (void)map;
    if (*size < 64) return EFI_BUFFER_TOO_SMALL;
    global_state->map_calls++;
    *key = global_state->map_calls;
    *descriptor_size = 48;
    *version = EFI_MEMORY_DESCRIPTOR_VERSION;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_exit_boot_services(EFI_HANDLE image, UINTN key) {
    exit_state *state = (exit_state *)(uintptr_t)image;
    (void)key;
    state->exit_calls++;
    return state->exit_calls == 1 ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

int main(void) {
    EFI_BOOT_SERVICES services = {0};
    exit_state state = {0, 0};
    uint8_t memory_map[128] = {0};
    global_state = &state;
    UINTN final_key = 0;
    services.GetMemoryMap = fake_get_memory_map;
    services.ExitBootServices = fake_exit_boot_services;
    assert(xboot_uefi_exit_boot_services((EFI_HANDLE)(uintptr_t)&state,
                                         &services, memory_map,
                                         sizeof(memory_map), 48,
                                         &final_key) == XBOOT_OK);
    assert(state.map_calls == 2);
    assert(state.exit_calls == 2);
    assert(final_key == 2);
    puts("ExitBootServices retry host tests: PASS");
    return 0;
}
