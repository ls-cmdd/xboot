#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/memory.h>

typedef struct {
    uint64_t next_address;
    uint64_t live_address;
    uint64_t live_pages;
    unsigned int alloc_calls;
    unsigned int free_calls;
} fake_page_backend;

static xboot_status_t test_allocate(void *context, uint64_t pages, uint64_t *out) {
    fake_page_backend *backend = context;
    if (pages == 0 || backend->live_address != 0) {
        return XBOOT_NO_MEMORY;
    }
    backend->live_address = backend->next_address;
    backend->live_pages = pages;
    backend->alloc_calls++;
    *out = backend->live_address;
    return XBOOT_OK;
}

static xboot_status_t test_free(void *context, uint64_t address, uint64_t pages) {
    fake_page_backend *backend = context;
    if (backend->live_address != address || backend->live_pages != pages) {
        return XBOOT_NOT_FOUND;
    }
    backend->live_address = 0;
    backend->live_pages = 0;
    backend->free_calls++;
    return XBOOT_OK;
}

int main(void) {
    fake_page_backend backend = {
        .next_address = 0x100000,
        .live_address = 0,
        .live_pages = 0,
        .alloc_calls = 0,
        .free_calls = 0,
    };
    xboot_memory_manager manager;
    xboot_page_backend page_backend = {
        .context = &backend,
        .allocate_pages = test_allocate,
        .free_pages = test_free,
    };
    uint64_t address = 0;

    assert(xboot_memory_manager_init(NULL, page_backend) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_memory_manager_init(&manager, page_backend) == XBOOT_OK);
    assert(xboot_page_alloc(&manager, 0, &address) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_page_alloc(&manager, 1, NULL) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_page_alloc(&manager, 1, &address) == XBOOT_OK);
    assert(address == 0x100000);
    assert(xboot_page_allocation_count(&manager) == 1);
    assert(xboot_page_free(&manager, address, 2) == XBOOT_NOT_FOUND);
    assert(xboot_page_free(&manager, address, 1) == XBOOT_OK);
    assert(xboot_page_allocation_count(&manager) == 0);
    assert(xboot_page_free(&manager, address, 1) == XBOOT_NOT_FOUND);
    assert(backend.alloc_calls == 1);
    assert(backend.free_calls == 1);

    puts("page allocator host tests: PASS");
    return 0;
}
