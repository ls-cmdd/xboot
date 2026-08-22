#ifndef XBOOT_RESOURCE_H
#define XBOOT_RESOURCE_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef enum {
    XBOOT_RESOURCE_MEMORY = 1,
    XBOOT_RESOURCE_MMIO = 2,
    XBOOT_RESOURCE_PCI = 3,
    XBOOT_RESOURCE_FILESYSTEM_HANDLE = 4,
    XBOOT_RESOURCE_STORAGE_QUEUE = 5,
    XBOOT_RESOURCE_FIRMWARE_HANDLE = 6,
} xboot_resource_kind;

typedef enum {
    XBOOT_RESOURCE_ACQUIRED = 1,
    XBOOT_RESOURCE_RELEASED = 2,
    XBOOT_RESOURCE_FAILED = 3,
} xboot_resource_state;

typedef xboot_status_t (*xboot_resource_cleanup)(void *context,
                                                  uint64_t address,
                                                  uint64_t size);

typedef struct {
    uint64_t id;
    xboot_resource_kind kind;
    xboot_resource_state state;
    const char *owner;
    uint64_t address;
    uint64_t size;
    void *context;
    xboot_resource_cleanup cleanup;
} xboot_resource_record;

#define XBOOT_MAX_RESOURCES 128U

typedef struct {
    xboot_resource_record records[XBOOT_MAX_RESOURCES];
    uint64_t next_id;
    size_t active_count;
} xboot_resource_manager;

xboot_status_t xboot_resource_manager_init(xboot_resource_manager *manager);
xboot_status_t xboot_resource_acquire(xboot_resource_manager *manager,
                                      xboot_resource_kind kind,
                                      const char *owner,
                                      uint64_t address,
                                      uint64_t size,
                                      void *context,
                                      xboot_resource_cleanup cleanup,
                                      uint64_t *out_id);
xboot_status_t xboot_resource_release(xboot_resource_manager *manager,
                                      uint64_t id);
const xboot_resource_record *xboot_resource_find(const xboot_resource_manager *manager,
                                                 uint64_t id);
size_t xboot_resource_active_count(const xboot_resource_manager *manager);

#endif
