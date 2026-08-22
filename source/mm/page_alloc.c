#include <xboot/memory.h>

#define XBOOT_PAGE_SIZE 4096ULL

static int allocation_matches(const xboot_page_allocation *allocation,
                              uint64_t address,
                              uint64_t page_count) {
    return allocation->address == address && allocation->page_count == page_count;
}

xboot_status_t xboot_memory_manager_init(xboot_memory_manager *manager,
                                          xboot_page_backend backend) {
    if (manager == NULL || backend.allocate_pages == NULL ||
        backend.free_pages == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    manager->backend = backend;
    manager->allocation_count = 0;
    for (size_t i = 0; i < XBOOT_MAX_PAGE_ALLOCATIONS; i++) {
        manager->allocations[i].address = 0;
        manager->allocations[i].page_count = 0;
    }
    return XBOOT_OK;
}

xboot_status_t xboot_page_alloc(xboot_memory_manager *manager,
                                uint64_t page_count,
                                uint64_t *out_address) {
    xboot_status_t status;
    uint64_t address = 0;
    if (manager == NULL || out_address == NULL || page_count == 0 ||
        page_count > UINT64_MAX / XBOOT_PAGE_SIZE) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if (manager->allocation_count >= XBOOT_MAX_PAGE_ALLOCATIONS) {
        return XBOOT_NO_MEMORY;
    }
    status = manager->backend.allocate_pages(manager->backend.context,
                                             page_count, &address);
    if (status != XBOOT_OK) {
        return status;
    }
    if (address == 0 || (address % XBOOT_PAGE_SIZE) != 0) {
        (void)manager->backend.free_pages(manager->backend.context,
                                          address, page_count);
        return XBOOT_PROTOCOL_ERROR;
    }
    manager->allocations[manager->allocation_count].address = address;
    manager->allocations[manager->allocation_count].page_count = page_count;
    manager->allocation_count++;
    *out_address = address;
    return XBOOT_OK;
}

xboot_status_t xboot_page_free(xboot_memory_manager *manager,
                               uint64_t address,
                               uint64_t page_count) {
    size_t index;
    xboot_status_t status;
    if (manager == NULL || address == 0 || page_count == 0 ||
        (address % XBOOT_PAGE_SIZE) != 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (index = 0; index < manager->allocation_count; index++) {
        if (allocation_matches(&manager->allocations[index], address, page_count)) {
            status = manager->backend.free_pages(manager->backend.context,
                                                 address, page_count);
            if (status != XBOOT_OK) {
                return status;
            }
            manager->allocation_count--;
            manager->allocations[index] =
                manager->allocations[manager->allocation_count];
            manager->allocations[manager->allocation_count].address = 0;
            manager->allocations[manager->allocation_count].page_count = 0;
            return XBOOT_OK;
        }
    }
    return XBOOT_NOT_FOUND;
}

size_t xboot_page_allocation_count(const xboot_memory_manager *manager) {
    return manager == NULL ? 0 : manager->allocation_count;
}
