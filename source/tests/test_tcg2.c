#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/uefi_tcg2.h>

static EFI_TCG2_PROTOCOL fake_protocol;
static uint8_t protocol_available;
static uint8_t tpm_available = 1;
static uint8_t event_called;
static uint32_t capability_hash_bitmap = EFI_TCG2_BOOT_HASH_ALG_SHA256;
static uint32_t capability_active_banks = EFI_TCG2_BOOT_HASH_ALG_SHA256;
static EFI_STATUS measurement_status = EFI_SUCCESS;
static uint64_t captured_address;
static uint64_t captured_size;
static EFI_TCG2_EVENT *captured_event;
static uint8_t captured_event_bytes[128];

static EFI_STATUS EFIAPI fake_locate(EFI_GUID *guid, void *registration, void **interface) {
    (void)registration;
    assert(guid->Data1 == 0x607F766C);
    if (protocol_available == 0) return EFI_NOT_FOUND;
    *interface = &fake_protocol;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_capability(EFI_TCG2_PROTOCOL *protocol,
                                         EFI_TCG2_BOOT_SERVICE_CAPABILITY *capability) {
    assert(protocol == &fake_protocol);
    capability->TPMPresentFlag = tpm_available;
    capability->HashAlgorithmBitmap = capability_hash_bitmap;
    capability->ActivePcrBanks = capability_active_banks;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI fake_measure(EFI_TCG2_PROTOCOL *protocol, uint64_t flags,
                                      EFI_PHYSICAL_ADDRESS address, uint64_t size,
                                      EFI_TCG2_EVENT *event) {
    assert(protocol == &fake_protocol);
    assert(flags == 0);
    event_called = 1;
    captured_address = address;
    captured_size = size;
    captured_event = event;
    assert(event->Size <= sizeof(captured_event_bytes));
    memcpy(captured_event_bytes, event, event->Size);
    return measurement_status;
}

int main(void) {
    EFI_BOOT_SERVICES boot_services;
    uint8_t data[] = {'k', 'e', 'r', 'n', 'e', 'l'};
    memset(&boot_services, 0, sizeof(boot_services));
    memset(&fake_protocol, 0, sizeof(fake_protocol));
    fake_protocol.GetCapability = fake_capability;
    fake_protocol.HashLogExtendEvent = fake_measure;
    boot_services.LocateProtocol = fake_locate;

    protocol_available = 0;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_NOT_FOUND);

    protocol_available = 1;
    tpm_available = 0;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_UNSUPPORTED);

    tpm_available = 1;
    capability_hash_bitmap = 0;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_UNSUPPORTED);
    capability_hash_bitmap = EFI_TCG2_BOOT_HASH_ALG_SHA256;
    capability_active_banks = 0;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_UNSUPPORTED);
    capability_active_banks = EFI_TCG2_BOOT_HASH_ALG_SHA256;
    event_called = 0;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_OK);
    assert(event_called == 1);
    assert(captured_address == (uint64_t)(uintptr_t)data);
    assert(captured_size == sizeof(data));
    assert(captured_event != NULL);
    captured_event = (EFI_TCG2_EVENT *)(void *)captured_event_bytes;
    assert(captured_event->Header.PCRIndex == 4);
    assert(captured_event->Header.EventType == EFI_TCG2_EVENT_TYPE_IPL);
    assert(captured_event->Header.HeaderVersion == EFI_TCG2_EVENT_HEADER_VERSION);
    assert(captured_event->Size == sizeof(uint32_t) + sizeof(EFI_TCG2_EVENT_HEADER) + 8U);
    assert(memcmp(captured_event->Event, "artifact", 8) == 0);
    measurement_status = EFI_VOLUME_FULL;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_RESOURCE_BUSY);
    measurement_status = EFI_DEVICE_ERROR;
    assert(xboot_uefi_tcg2_measure(&boot_services, data, sizeof(data), "artifact") == XBOOT_HARDWARE_ERROR);
    measurement_status = EFI_SUCCESS;
    puts("TCG2 adapter host tests: PASS");
    return 0;
}
