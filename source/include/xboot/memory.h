#ifndef XBOOT_MEMORY_H
#define XBOOT_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef xboot_status_t (*xboot_page_backend_alloc)(void *context,
                                                     uint64_t page_count,
                                                     uint64_t *out_address);
typedef xboot_status_t (*xboot_page_backend_free)(void *context,
                                                    uint64_t address,
                                                    uint64_t page_count);

typedef struct {
    void *context;
    xboot_page_backend_alloc allocate_pages;
    xboot_page_backend_free free_pages;
} xboot_page_backend;

typedef struct {
    uint64_t address;
    uint64_t page_count;
} xboot_page_allocation;

#define XBOOT_MAX_PAGE_ALLOCATIONS 64U

typedef struct {
    xboot_page_backend backend;
    xboot_page_allocation allocations[XBOOT_MAX_PAGE_ALLOCATIONS];
    size_t allocation_count;
} xboot_memory_manager;

xboot_status_t xboot_memory_manager_init(xboot_memory_manager *manager,
                                          xboot_page_backend backend);
xboot_status_t xboot_page_alloc(xboot_memory_manager *manager,
                                uint64_t page_count,
                                uint64_t *out_address);
xboot_status_t xboot_page_free(xboot_memory_manager *manager,
                               uint64_t address,
                               uint64_t page_count);
size_t xboot_page_allocation_count(const xboot_memory_manager *manager);

#endif
