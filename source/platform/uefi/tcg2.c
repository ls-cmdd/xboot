#include <xboot/uefi_tcg2.h>

static size_t ascii_length(const char *value) {
    size_t length = 0;
    while (value[length] != '\0') length++;
    return length;
}

xboot_status_t xboot_uefi_tcg2_measure(EFI_BOOT_SERVICES *boot_services,
                                       const uint8_t *data, size_t data_size,
                                       const char *event_description) {
    EFI_GUID protocol_guid = EFI_TCG2_PROTOCOL_GUID;
    EFI_TCG2_PROTOCOL *protocol = NULL;
    EFI_TCG2_BOOT_SERVICE_CAPABILITY capability;
    struct {
        EFI_TCG2_EVENT event;
        uint8_t description[64];
    } event_buffer;
    size_t description_size;
    EFI_STATUS firmware_status;
    if (boot_services == NULL || boot_services->LocateProtocol == NULL ||
        data == NULL || data_size == 0 || event_description == NULL) return XBOOT_INVALID_ARGUMENT;
    firmware_status = boot_services->LocateProtocol(&protocol_guid, NULL, (void **)&protocol);
    if (firmware_status == EFI_NOT_FOUND || protocol == NULL) return XBOOT_NOT_FOUND;
    if (firmware_status != EFI_SUCCESS || protocol->GetCapability == NULL ||
        protocol->HashLogExtendEvent == NULL) return XBOOT_UNSUPPORTED;
    for (size_t i = 0; i < sizeof(capability); i++) ((uint8_t *)&capability)[i] = 0;
    capability.Size = (uint8_t)sizeof(capability);
    firmware_status = protocol->GetCapability(protocol, &capability);
    if (firmware_status != EFI_SUCCESS) return XBOOT_HARDWARE_ERROR;
    if (capability.TPMPresentFlag == 0 ||
        (capability.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SHA256) == 0 ||
        (capability.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA256) == 0) return XBOOT_UNSUPPORTED;
    description_size = ascii_length(event_description);
    if (description_size == 0 || description_size > sizeof(event_buffer.description)) return XBOOT_BUFFER_TOO_SMALL;
    for (size_t i = 0; i < description_size; i++) {
        ((uint8_t *)&event_buffer.event)[sizeof(uint32_t) + sizeof(EFI_TCG2_EVENT_HEADER) + i] =
            (uint8_t)event_description[i];
    }
    event_buffer.event.Size = (uint32_t)(sizeof(uint32_t) + sizeof(EFI_TCG2_EVENT_HEADER) + description_size);
    event_buffer.event.Header.HeaderSize = (uint32_t)sizeof(EFI_TCG2_EVENT_HEADER);
    event_buffer.event.Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
    event_buffer.event.Header.PCRIndex = 4U;
    event_buffer.event.Header.EventType = EFI_TCG2_EVENT_TYPE_IPL;
    firmware_status = protocol->HashLogExtendEvent(protocol, 0, (EFI_PHYSICAL_ADDRESS)(uintptr_t)data,
                                                   (uint64_t)data_size, &event_buffer.event);
    if (firmware_status == EFI_SUCCESS) return XBOOT_OK;
    if (firmware_status == EFI_VOLUME_FULL) return XBOOT_RESOURCE_BUSY;
    if (firmware_status == EFI_NOT_FOUND || firmware_status == EFI_UNSUPPORTED) return XBOOT_UNSUPPORTED;
    return XBOOT_HARDWARE_ERROR;
}
