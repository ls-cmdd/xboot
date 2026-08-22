#include <xboot/resource.h>

xboot_status_t xboot_resource_manager_init(xboot_resource_manager *manager) {
    if (manager == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    manager->next_id = 1;
    manager->active_count = 0;
    for (size_t i = 0; i < XBOOT_MAX_RESOURCES; i++) {
        manager->records[i].id = 0;
        manager->records[i].state = XBOOT_RESOURCE_RELEASED;
    }
    return XBOOT_OK;
}

xboot_status_t xboot_resource_acquire(xboot_resource_manager *manager,
                                      xboot_resource_kind kind,
                                      const char *owner,
                                      uint64_t address,
                                      uint64_t size,
                                      void *context,
                                      xboot_resource_cleanup cleanup,
                                      uint64_t *out_id) {
    size_t index;
    if (manager == NULL || owner == NULL || out_id == NULL || kind == 0 ||
        size == 0 || manager->active_count >= XBOOT_MAX_RESOURCES) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (index = 0; index < XBOOT_MAX_RESOURCES; index++) {
        if (manager->records[index].state == XBOOT_RESOURCE_RELEASED) {
            break;
        }
    }
    if (index == XBOOT_MAX_RESOURCES) {
        return XBOOT_NO_MEMORY;
    }
    if (manager->next_id == 0) {
        return XBOOT_NO_MEMORY;
    }
    manager->records[index].id = manager->next_id++;
    manager->records[index].kind = kind;
    manager->records[index].state = XBOOT_RESOURCE_ACQUIRED;
    manager->records[index].owner = owner;
    manager->records[index].address = address;
    manager->records[index].size = size;
    manager->records[index].context = context;
    manager->records[index].cleanup = cleanup;
    manager->active_count++;
    *out_id = manager->records[index].id;
    return XBOOT_OK;
}

xboot_status_t xboot_resource_release(xboot_resource_manager *manager,
                                      uint64_t id) {
    xboot_resource_record *record;
    xboot_status_t status = XBOOT_OK;
    if (manager == NULL || id == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    record = (xboot_resource_record *)xboot_resource_find(manager, id);
    if (record == NULL) {
        return XBOOT_NOT_FOUND;
    }
    if (record->state != XBOOT_RESOURCE_ACQUIRED) {
        return XBOOT_RESOURCE_BUSY;
    }
    if (record->cleanup != NULL) {
        status = record->cleanup(record->context, record->address, record->size);
    }
    if (status != XBOOT_OK) {
        record->state = XBOOT_RESOURCE_FAILED;
        return status;
    }
    record->state = XBOOT_RESOURCE_RELEASED;
    record->owner = NULL;
    record->address = 0;
    record->size = 0;
    record->context = NULL;
    record->cleanup = NULL;
    manager->active_count--;
    return XBOOT_OK;
}

const xboot_resource_record *xboot_resource_find(const xboot_resource_manager *manager,
                                                 uint64_t id) {
    if (manager == NULL || id == 0) {
        return NULL;
    }
    for (size_t i = 0; i < XBOOT_MAX_RESOURCES; i++) {
        if (manager->records[i].id == id && manager->records[i].state != XBOOT_RESOURCE_RELEASED) {
            return &manager->records[i];
        }
    }
    return NULL;
}

size_t xboot_resource_active_count(const xboot_resource_manager *manager) {
    return manager == NULL ? 0 : manager->active_count;
}
