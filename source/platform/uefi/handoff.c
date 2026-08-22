#include <xboot/handoff.h>

static int guid_equal(const EFI_GUID *a, const EFI_GUID *b) {
    unsigned int i;
    if (a->Data1 != b->Data1 || a->Data2 != b->Data2 || a->Data3 != b->Data3) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        if (a->Data4[i] != b->Data4[i]) {
            return 0;
        }
    }
    return 1;
}

static xboot_status_t map_uefi_status(EFI_STATUS status) {
    if (status == EFI_SUCCESS) {
        return XBOOT_OK;
    }
    if (status == EFI_BUFFER_TOO_SMALL || status == EFI_OUT_OF_RESOURCES) {
        return XBOOT_NO_MEMORY;
    }
    return XBOOT_IO_ERROR;
}

static void *find_configuration_table(EFI_SYSTEM_TABLE *system_table,
                                       const EFI_GUID *first,
                                       const EFI_GUID *second) {
    UINTN i;
    if (system_table->ConfigurationTable == NULL) {
        return NULL;
    }
    for (i = 0; i < system_table->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *entry = &system_table->ConfigurationTable[i];
        if (guid_equal(&entry->VendorGuid, first) ||
            guid_equal(&entry->VendorGuid, second)) {
            return entry->VendorTable;
        }
    }
    return NULL;
}

xboot_status_t xboot_uefi_collect_handoff(EFI_SYSTEM_TABLE *system_table,
                                           EFI_BOOT_SERVICES *boot_services,
                                           xboot_uefi_handoff **out_handoff) {
    EFI_STATUS efi_status;
    EFI_GUID acpi20 = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID acpi10 = EFI_ACPI_TABLE_GUID;
    EFI_GUID smbios = SMBIOS_TABLE_GUID;
    EFI_GUID smbios3 = SMBIOS3_TABLE_GUID;
    xboot_uefi_handoff *handoff;
    EFI_MEMORY_DESCRIPTOR *raw_map;
    UINTN map_size = 0;
    UINTN map_key = 0;
    UINTN descriptor_size = 0;
    uint32_t descriptor_version = 0;
    UINTN map_capacity;
    UINTN offset;
    size_t region_count = 0;

    if (system_table == NULL || boot_services == NULL || out_handoff == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    *out_handoff = NULL;

    efi_status = boot_services->AllocatePool(EFI_LOADER_DATA, sizeof(*handoff),
                                             (void **)&handoff);
    if (efi_status != EFI_SUCCESS) {
        return map_uefi_status(efi_status);
    }
    handoff->memory_regions = NULL;
    handoff->memory_region_count = 0;
    handoff->memory_descriptor_size = 0;
    handoff->memory_descriptor_version = 0;
    handoff->uefi_map_key = 0;
    handoff->acpi_table = find_configuration_table(system_table, &acpi20, &acpi10);
    handoff->smbios_table = find_configuration_table(system_table, &smbios, &smbios3);
    handoff->firmware_revision = system_table->FirmwareRevision;
    handoff->system_table_revision = system_table->Hdr.Revision;
    handoff->uefi_memory_map = NULL;
    handoff->uefi_memory_map_capacity = 0;

    efi_status = boot_services->GetMemoryMap(&map_size, NULL, &map_key,
                                             &descriptor_size, &descriptor_version);
    if (efi_status != EFI_BUFFER_TOO_SMALL || descriptor_size < sizeof(EFI_MEMORY_DESCRIPTOR)) {
        (void)boot_services->FreePool(handoff);
        return map_uefi_status(efi_status);
    }
    if (descriptor_size > (SIZE_MAX - map_size) / 8U) {
        (void)boot_services->FreePool(handoff);
        return XBOOT_NO_MEMORY;
    }
    map_capacity = map_size + descriptor_size * 8U;
    efi_status = boot_services->AllocatePool(EFI_LOADER_DATA, map_capacity,
                                             (void **)&raw_map);
    if (efi_status != EFI_SUCCESS) {
        (void)boot_services->FreePool(handoff);
        return map_uefi_status(efi_status);
    }
    map_size = map_capacity;
    efi_status = boot_services->GetMemoryMap(&map_size, raw_map, &map_key,
                                             &descriptor_size, &descriptor_version);
    if (efi_status != EFI_SUCCESS) {
        (void)boot_services->FreePool(raw_map);
        (void)boot_services->FreePool(handoff);
        return map_uefi_status(efi_status);
    }
    for (offset = 0; offset + descriptor_size <= map_size; offset += descriptor_size) {
        region_count++;
    }
    if (region_count == 0 || region_count > SIZE_MAX / sizeof(xboot_memory_region)) {
        (void)boot_services->FreePool(raw_map);
        (void)boot_services->FreePool(handoff);
        return XBOOT_CORRUPT;
    }
    efi_status = boot_services->AllocatePool(
        EFI_LOADER_DATA, (UINTN)(region_count * sizeof(xboot_memory_region)),
        (void **)&handoff->memory_regions);
    if (efi_status != EFI_SUCCESS) {
        (void)boot_services->FreePool(raw_map);
        (void)boot_services->FreePool(handoff);
        return map_uefi_status(efi_status);
    }
    region_count = 0;
    for (offset = 0; offset + descriptor_size <= map_size; offset += descriptor_size) {
        EFI_MEMORY_DESCRIPTOR *descriptor =
            (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)raw_map + offset);
        xboot_memory_region *region = &handoff->memory_regions[region_count++];
        region->type = descriptor->Type;
        region->physical_start = descriptor->PhysicalStart;
        region->virtual_start = descriptor->VirtualStart;
        region->page_count = descriptor->NumberOfPages;
        region->attribute = descriptor->Attribute;
    }
    handoff->memory_region_count = region_count;
    handoff->memory_descriptor_size = descriptor_size;
    handoff->memory_descriptor_version = descriptor_version;
    handoff->uefi_map_key = map_key;
    handoff->uefi_memory_map = raw_map;
    handoff->uefi_memory_map_capacity = map_capacity;
    *out_handoff = handoff;
    return XBOOT_OK;
}

xboot_status_t xboot_uefi_expand_memory_map(EFI_BOOT_SERVICES *boot_services,
                                             xboot_uefi_handoff *handoff,
                                             UINTN additional_descriptors) {
    UINTN extra_bytes;
    UINTN new_capacity;
    void *new_map = NULL;
    EFI_STATUS status;
    if (boot_services == NULL || handoff == NULL || handoff->uefi_memory_map == NULL ||
        handoff->memory_descriptor_size == 0 || additional_descriptors == 0) {
        return XBOOT_INVALID_ARGUMENT;
    }
    if (additional_descriptors > UINT64_MAX / handoff->memory_descriptor_size) {
        return XBOOT_NO_MEMORY;
    }
    extra_bytes = additional_descriptors * handoff->memory_descriptor_size;
    if (handoff->uefi_memory_map_capacity > UINT64_MAX - extra_bytes) {
        return XBOOT_NO_MEMORY;
    }
    new_capacity = handoff->uefi_memory_map_capacity + extra_bytes;
    status = boot_services->AllocatePool(EFI_LOADER_DATA, new_capacity, &new_map);
    if (status != EFI_SUCCESS || new_map == NULL) {
        return status == EFI_OUT_OF_RESOURCES ? XBOOT_NO_MEMORY : XBOOT_IO_ERROR;
    }
    for (UINTN i = 0; i < handoff->uefi_memory_map_capacity; i++) {
        ((uint8_t *)new_map)[i] = ((const uint8_t *)handoff->uefi_memory_map)[i];
    }
    (void)boot_services->FreePool(handoff->uefi_memory_map);
    handoff->uefi_memory_map = new_map;
    handoff->uefi_memory_map_capacity = new_capacity;
    return XBOOT_OK;
}

void xboot_uefi_release_handoff(EFI_BOOT_SERVICES *boot_services,
                                xboot_uefi_handoff *handoff) {
    void *raw_map;
    if (boot_services == NULL || handoff == NULL) {
        return;
    }
    raw_map = handoff->uefi_memory_map;
    if (handoff->memory_regions != NULL) {
        (void)boot_services->FreePool(handoff->memory_regions);
    }
    if (raw_map != NULL) {
        (void)boot_services->FreePool(raw_map);
    }
    (void)boot_services->FreePool(handoff);
}
