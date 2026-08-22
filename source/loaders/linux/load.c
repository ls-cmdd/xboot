#include <xboot/linux_loader.h>

static void clear_bytes(void *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) ((uint8_t *)buffer)[i] = 0;
}

static void copy_bytes(void *destination, const void *source, size_t size) {
    for (size_t i = 0; i < size; i++) ((uint8_t *)destination)[i] = ((const uint8_t *)source)[i];
}

static int add_overflow(uint64_t a, uint64_t b, uint64_t *out) {
    if (b > UINT64_MAX - a) return 1;
    *out = a + b;
    return 0;
}

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1U) & ~(alignment - 1U);
}

static uint64_t pages_for(uint64_t size) {
    return (size + XBOOT_LINUX_PAGE_SIZE - 1U) / XBOOT_LINUX_PAGE_SIZE;
}

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void put64(uint8_t *p, uint64_t value) {
    put32(p, (uint32_t)value);
    put32(p + 4, (uint32_t)(value >> 32));
}

static xboot_status_t alloc_aligned(xboot_memory_manager *memory, uint64_t size,
                                    uint64_t alignment, uint64_t *out_base,
                                    uint64_t *out_pages, uint64_t *out_address) {
    uint64_t pages;
    uint64_t extra_pages;
    uint64_t base;
    uint64_t end;
    if (memory == NULL || out_base == NULL || out_pages == NULL || out_address == NULL ||
        size == 0 || alignment == 0 || (alignment & (alignment - 1U)) != 0) return XBOOT_INVALID_ARGUMENT;
    if (add_overflow(size, XBOOT_LINUX_PAGE_SIZE - 1U, &end)) return XBOOT_NO_MEMORY;
    pages = end / XBOOT_LINUX_PAGE_SIZE;
    extra_pages = alignment / XBOOT_LINUX_PAGE_SIZE;
    if (extra_pages > 0) {
        if (add_overflow(pages, extra_pages - 1U, &pages)) return XBOOT_NO_MEMORY;
    }
    if (xboot_page_alloc(memory, pages, &base) != XBOOT_OK) return XBOOT_NO_MEMORY;
    end = align_up(base, alignment);
    if (end < base || end - base > (pages - 1U) * XBOOT_LINUX_PAGE_SIZE) {
        (void)xboot_page_free(memory, base, pages);
        return XBOOT_PROTOCOL_ERROR;
    }
    *out_base = base;
    *out_pages = pages;
    *out_address = end;
    return XBOOT_OK;
}

static void release_if(xboot_memory_manager *memory, uint64_t address, uint64_t pages) {
    if (address != 0 && pages != 0) (void)xboot_page_free(memory, address, pages);
}

xboot_status_t xboot_linux_load_plan(const uint8_t *image, size_t image_size,
                                     const uint8_t *initrd, size_t initrd_size,
                                     const char *cmdline, xboot_memory_manager *memory,
                                     xboot_linux_boot_plan *out_plan) {
    xboot_linux_image_info info;
    uint64_t protected_size;
    uint64_t kernel_footprint;
    uint64_t kernel_end;
    uint64_t cmdline_length = 0;
    const char *effective_cmdline = cmdline;
    xboot_status_t status;
    if (image == NULL || memory == NULL || out_plan == NULL) return XBOOT_INVALID_ARGUMENT;
    clear_bytes(out_plan, sizeof(*out_plan));
    status = xboot_linux_inspect(image, image_size, &info);
    if (status != XBOOT_OK) return status;
    if (info.relocatable_kernel == 0) return XBOOT_UNSUPPORTED;
    protected_size = info.protected_size;
    kernel_footprint = protected_size > info.init_size ? protected_size : info.init_size;
    if (protected_size == 0 || kernel_footprint == 0 || info.kernel_alignment == 0 ||
        (info.xloadflags & 1U) == 0) return XBOOT_CORRUPT;
    if (effective_cmdline == NULL || effective_cmdline[0] == '\0') effective_cmdline = "auto";
    while (effective_cmdline[cmdline_length] != '\0') {
        if (cmdline_length + 1U >= XBOOT_LINUX_MAX_CMDLINE) return XBOOT_BUFFER_TOO_SMALL;
        cmdline_length++;
    }
    status = alloc_aligned(memory, kernel_footprint, info.kernel_alignment,
                           &out_plan->kernel_allocation_address,
                           &out_plan->kernel_allocation_pages,
                           &out_plan->kernel_address);
    if (status != XBOOT_OK) return status;
    if (add_overflow(out_plan->kernel_address, kernel_footprint, &kernel_end) || kernel_end > UINT32_MAX) {
        release_if(memory, out_plan->kernel_allocation_address, out_plan->kernel_allocation_pages);
        clear_bytes(out_plan, sizeof(*out_plan));
        return XBOOT_NO_MEMORY;
    }
    copy_bytes((void *)(uintptr_t)out_plan->kernel_address, image + info.setup_size, (size_t)protected_size);
    if (initrd != NULL && initrd_size != 0) {
        if (info.initrd_addr_max == 0) { release_if(memory, out_plan->kernel_allocation_address, out_plan->kernel_allocation_pages); clear_bytes(out_plan, sizeof(*out_plan)); return XBOOT_UNSUPPORTED; }
        status = alloc_aligned(memory, initrd_size, XBOOT_LINUX_PAGE_SIZE,
                               &out_plan->initrd_allocation_address,
                               &out_plan->initrd_allocation_pages,
                               &out_plan->initrd_address);
        if (status != XBOOT_OK) { release_if(memory, out_plan->kernel_allocation_address, out_plan->kernel_allocation_pages); clear_bytes(out_plan, sizeof(*out_plan)); return status; }
        if (out_plan->initrd_address > info.initrd_addr_max ||
            initrd_size - 1U > info.initrd_addr_max - out_plan->initrd_address) {
            release_if(memory, out_plan->initrd_allocation_address, out_plan->initrd_allocation_pages);
            release_if(memory, out_plan->kernel_allocation_address, out_plan->kernel_allocation_pages);
            clear_bytes(out_plan, sizeof(*out_plan));
            return XBOOT_NO_MEMORY;
        }
        copy_bytes((void *)(uintptr_t)out_plan->initrd_address, initrd, initrd_size);
        out_plan->initrd_size = initrd_size;
        out_plan->has_initrd = 1;
    }
    out_plan->boot_params_pages = pages_for(info.setup_size);
    if (out_plan->boot_params_pages == 0) { xboot_linux_release_plan(memory, out_plan); return XBOOT_CORRUPT; }
    status = xboot_page_alloc(memory, out_plan->boot_params_pages, &out_plan->boot_params_address);
    if (status != XBOOT_OK) { xboot_linux_release_plan(memory, out_plan); return status; }
    clear_bytes((void *)(uintptr_t)out_plan->boot_params_address,
                (size_t)(out_plan->boot_params_pages * XBOOT_LINUX_PAGE_SIZE));
    copy_bytes((void *)(uintptr_t)out_plan->boot_params_address, image, (size_t)info.setup_size);
    out_plan->setup_address = out_plan->boot_params_address;
    status = xboot_page_alloc(memory, 1, &out_plan->cmdline_address);
    if (status != XBOOT_OK) { xboot_linux_release_plan(memory, out_plan); return status; }
    clear_bytes((void *)(uintptr_t)out_plan->cmdline_address, XBOOT_LINUX_PAGE_SIZE);
    copy_bytes((void *)(uintptr_t)out_plan->cmdline_address, effective_cmdline, (size_t)cmdline_length);

    out_plan->kernel_size = kernel_footprint;
    out_plan->prepared = 1;
    return XBOOT_OK;
}

xboot_status_t xboot_linux_build_boot_params(const uint8_t *image, size_t image_size,
                                              const xboot_uefi_handoff *handoff,
                                              xboot_linux_boot_plan *plan) {
    xboot_linux_image_info info;
    uint8_t *params;
    uint32_t e820_count = 0;
    uint64_t pending_start = 0;
    uint64_t pending_bytes = 0;
    uint32_t pending_type = 0;
    uint8_t pending_valid = 0;
    if (image == NULL || handoff == NULL || plan == NULL || plan->prepared == 0 ||
        plan->boot_params_address == 0 || plan->cmdline_address == 0) return XBOOT_INVALID_ARGUMENT;
    if (xboot_linux_inspect(image, image_size, &info) != XBOOT_OK) return XBOOT_CORRUPT;
    if (plan->boot_params_address > UINT32_MAX || plan->cmdline_address > UINT32_MAX ||
        plan->kernel_address > UINT32_MAX) return XBOOT_UNSUPPORTED;
    if (plan->has_initrd && (plan->initrd_address > UINT32_MAX || plan->initrd_size > UINT32_MAX)) return XBOOT_UNSUPPORTED;
    params = (uint8_t *)(uintptr_t)plan->boot_params_address;
    params[0x210] = 0xFFU;
    params[0x211] = (uint8_t)(image[0x211] | 0x81U);
    put32(params + 0x214, (uint32_t)plan->kernel_address);
    put32(params + 0x218, plan->has_initrd ? (uint32_t)plan->initrd_address : 0U);
    put32(params + 0x21C, plan->has_initrd ? (uint32_t)plan->initrd_size : 0U);
    put32(params + 0x228, (uint32_t)plan->cmdline_address);
    put32(params + 0x22C, info.initrd_addr_max);
    put32(params + 0x230, info.kernel_alignment);
    params[0x234] = info.relocatable_kernel;
    put16(params + 0x236, info.xloadflags);
    put32(params + 0x238, info.cmdline_size);
    put64(params + 0x70, (uint64_t)(uintptr_t)handoff->acpi_table);
    for (size_t i = 0; i < handoff->memory_region_count; i++) {
        const xboot_memory_region *region = &handoff->memory_regions[i];
        uint64_t bytes;
        uint32_t type;
        if (region->page_count == 0) continue;
        if (region->page_count > UINT64_MAX / 4096ULL) return XBOOT_UNSUPPORTED;
        bytes = region->page_count * 4096ULL;
        type = (region->type == EFI_CONVENTIONAL_MEMORY || region->type == EFI_LOADER_DATA) ? 1U : 2U;
        if (pending_valid != 0 && pending_type == type &&
            pending_start <= UINT64_MAX - pending_bytes &&
            pending_start + pending_bytes == region->physical_start &&
            pending_bytes <= UINT64_MAX - bytes) {
            pending_bytes += bytes;
            continue;
        }
        if (pending_valid != 0) {
            uint8_t *entry;
            if (e820_count >= 128U) return XBOOT_UNSUPPORTED;
            entry = params + 0x2D0U + e820_count * 20U;
            put64(entry, pending_start);
            put64(entry + 8, pending_bytes);
            put32(entry + 16, pending_type);
            e820_count++;
        }
        pending_start = region->physical_start;
        pending_bytes = bytes;
        pending_type = type;
        pending_valid = 1;
    }
    if (pending_valid != 0) {
        uint8_t *entry;
        if (e820_count >= 128U) return XBOOT_UNSUPPORTED;
        entry = params + 0x2D0U + e820_count * 20U;
        put64(entry, pending_start);
        put64(entry + 8, pending_bytes);
        put32(entry + 16, pending_type);
        e820_count++;
    }
    params[0x1E8] = (uint8_t)e820_count;
    plan->e820_entries = e820_count;
    plan->boot_params_built = 1;
    return XBOOT_OK;
}

xboot_status_t xboot_linux_release_plan(xboot_memory_manager *memory,
                                        xboot_linux_boot_plan *plan) {
    xboot_status_t first_error = XBOOT_OK;
    xboot_status_t status;
    if (memory == NULL || plan == NULL) return XBOOT_INVALID_ARGUMENT;
    if (plan->direct_stack_address != 0) { status = xboot_page_free(memory, plan->direct_stack_address, plan->direct_stack_pages); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (plan->direct_page_table_address != 0) { status = xboot_page_free(memory, plan->direct_page_table_address, plan->direct_page_table_pages); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (plan->cmdline_address != 0) { status = xboot_page_free(memory, plan->cmdline_address, 1); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (plan->boot_params_address != 0) { status = xboot_page_free(memory, plan->boot_params_address, plan->boot_params_pages); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (plan->initrd_allocation_address != 0) { status = xboot_page_free(memory, plan->initrd_allocation_address, plan->initrd_allocation_pages); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (plan->kernel_allocation_address != 0) { status = xboot_page_free(memory, plan->kernel_allocation_address, plan->kernel_allocation_pages); if (status != XBOOT_OK && first_error == XBOOT_OK) first_error = status; }
    if (first_error == XBOOT_OK) clear_bytes(plan, sizeof(*plan));
    return first_error;
}
