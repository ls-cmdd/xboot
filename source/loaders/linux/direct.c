#include <xboot/linux_loader.h>

extern void xboot_linux_enter_64(uint64_t entry,
                                 uint64_t boot_params,
                                 uint64_t page_table,
                                 uint64_t stack_top);
extern xboot_status_t xboot_uefi_exit_boot_services(EFI_HANDLE image_handle,
                                                     EFI_BOOT_SERVICES *boot_services,
                                                     void *memory_map,
                                                     UINTN memory_map_capacity,
                                                     UINTN descriptor_size,
                                                     UINTN *out_map_key);

static void clear_pages(uint64_t address, uint64_t pages) {
    uint8_t *bytes = (uint8_t *)(uintptr_t)address;
    uint64_t size = pages * XBOOT_LINUX_PAGE_SIZE;
    for (uint64_t i = 0; i < size; i++) bytes[i] = 0;
}

static void put64(uint8_t *address, uint64_t value) {
    address[0] = (uint8_t)value;
    address[1] = (uint8_t)(value >> 8);
    address[2] = (uint8_t)(value >> 16);
    address[3] = (uint8_t)(value >> 24);
    address[4] = (uint8_t)(value >> 32);
    address[5] = (uint8_t)(value >> 40);
    address[6] = (uint8_t)(value >> 48);
    address[7] = (uint8_t)(value >> 56);
}

static void build_identity_map(uint64_t table_address) {
    uint8_t *pml4 = (uint8_t *)(uintptr_t)table_address;
    uint8_t *pdpt = pml4 + XBOOT_LINUX_PAGE_SIZE;
    uint8_t *directory = pdpt + XBOOT_LINUX_PAGE_SIZE;
    put64(pml4, table_address + XBOOT_LINUX_PAGE_SIZE + 3U);
    for (uint64_t i = 0; i < 4; i++) {
        uint64_t directory_address = table_address + 2U * XBOOT_LINUX_PAGE_SIZE + i * XBOOT_LINUX_PAGE_SIZE;
        put64(pdpt + i * 8U, directory_address + 3U);
        for (uint64_t j = 0; j < 512; j++) {
            uint64_t physical = (i * 512U + j) * 0x200000ULL;
            put64(directory + (i * 512U + j) * 8U, physical | 0x83U);
        }
    }
}

static xboot_status_t release_direct_allocations(xboot_memory_manager *memory,
                                                   xboot_linux_boot_plan *plan) {
    xboot_status_t first_error = XBOOT_OK;
    xboot_status_t status;
    if (plan->direct_stack_address != 0) {
        status = xboot_page_free(memory, plan->direct_stack_address, plan->direct_stack_pages);
        if (status != XBOOT_OK) first_error = status;
        plan->direct_stack_address = 0;
        plan->direct_stack_pages = 0;
    }
    if (plan->direct_page_table_address != 0) {
        status = xboot_page_free(memory, plan->direct_page_table_address,
                                 plan->direct_page_table_pages);
        if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status;
        plan->direct_page_table_address = 0;
        plan->direct_page_table_pages = 0;
    }
    return first_error;
}

xboot_status_t xboot_linux_direct_start(EFI_HANDLE image_handle,
                                         EFI_BOOT_SERVICES *boot_services,
                                         xboot_uefi_handoff *handoff,
                                         xboot_memory_manager *memory,
                                         xboot_linux_boot_plan *plan) {
    uint64_t kernel_end;
    uint64_t stack_top;
    UINTN final_key = 0;
    xboot_status_t status;
    if (image_handle == NULL || boot_services == NULL || handoff == NULL || memory == NULL ||
        plan == NULL || plan->prepared == 0 || plan->boot_params_built == 0 ||
        plan->kernel_address == 0 || plan->kernel_size < 0x200U ||
        plan->boot_params_address == 0 || plan->cmdline_address == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if (plan->kernel_address > UINT32_MAX || plan->boot_params_address > UINT32_MAX ||
        plan->cmdline_address > UINT32_MAX ||
        (plan->has_initrd && plan->initrd_address > UINT32_MAX) ||
        plan->kernel_size > UINT64_MAX - plan->kernel_address) {
        return XBOOT_UNSUPPORTED;
    }
    kernel_end = plan->kernel_address + plan->kernel_size;
    if (kernel_end > 0x100000000ULL) return XBOOT_UNSUPPORTED;
    status = xboot_uefi_expand_memory_map(boot_services, handoff, 32U);
    if (status != XBOOT_OK) return status;
    status = xboot_page_alloc(memory, 6U, &plan->direct_page_table_address);
    if (status != XBOOT_OK) return status;
    plan->direct_page_table_pages = 6U;
    if (plan->direct_page_table_address > UINT32_MAX) {
        (void)release_direct_allocations(memory, plan);
        return XBOOT_UNSUPPORTED;
    }
    clear_pages(plan->direct_page_table_address, plan->direct_page_table_pages);
    build_identity_map(plan->direct_page_table_address);
    status = xboot_page_alloc(memory, 2U, &plan->direct_stack_address);
    if (status != XBOOT_OK) {
        (void)release_direct_allocations(memory, plan);
        return status;
    }
    plan->direct_stack_pages = 2U;
    if (plan->direct_stack_address > UINT32_MAX ||
        plan->direct_stack_address > UINT64_MAX - 2U * XBOOT_LINUX_PAGE_SIZE) {
        (void)release_direct_allocations(memory, plan);
        return XBOOT_UNSUPPORTED;
    }
    clear_pages(plan->direct_stack_address, plan->direct_stack_pages);
    stack_top = plan->direct_stack_address + 2U * XBOOT_LINUX_PAGE_SIZE - 16U;
    status = xboot_uefi_exit_boot_services(image_handle, boot_services,
                                            handoff->uefi_memory_map,
                                            handoff->uefi_memory_map_capacity,
                                            handoff->memory_descriptor_size,
                                            &final_key);
    if (status != XBOOT_OK) {
        (void)release_direct_allocations(memory, plan);
        return status;
    }
    (void)final_key;
    xboot_linux_enter_64(plan->kernel_address + 0x200U,
                         plan->boot_params_address,
                         plan->direct_page_table_address,
                         stack_top);
    return XBOOT_PROTOCOL_ERROR;
}
