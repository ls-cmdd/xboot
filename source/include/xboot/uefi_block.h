#ifndef XBOOT_UEFI_BLOCK_H
#define XBOOT_UEFI_BLOCK_H

#include <xboot/storage.h>
#include <xboot/uefi.h>

xboot_status_t xboot_uefi_block_from_device(EFI_BOOT_SERVICES *boot_services,
                                            EFI_HANDLE device_handle,
                                            xboot_block_device *out_device);

xboot_status_t xboot_uefi_block_locate_first(EFI_BOOT_SERVICES *boot_services,
                                             xboot_block_device *out_device,
                                             EFI_HANDLE *out_handle);

#endif
