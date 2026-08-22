#ifndef XBOOT_UEFI_TCG2_H
#define XBOOT_UEFI_TCG2_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/uefi.h>

xboot_status_t xboot_uefi_tcg2_measure(EFI_BOOT_SERVICES *boot_services,
                                       const uint8_t *data, size_t data_size,
                                       const char *event_description);

#endif
