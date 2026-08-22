#ifndef XBOOT_UEFI_LINUX_H
#define XBOOT_UEFI_LINUX_H

#include <stddef.h>
#include <stdint.h>
#include <xboot/status.h>
#include <xboot/uefi.h>

xboot_status_t xboot_uefi_start_linux_file(EFI_BOOT_SERVICES *boot_services,
                                           EFI_HANDLE parent_image,
                                           const CHAR16 *file_path,
                                           const CHAR16 *load_options,
                                           UINTN load_options_size,
                                           EFI_HANDLE *out_image,
                                           EFI_STATUS *out_efi_status,
                                           uint32_t *out_stage);

xboot_status_t xboot_uefi_authenticate_linux_file(EFI_BOOT_SERVICES *boot_services,
                                                  EFI_HANDLE parent_image,
                                                  const CHAR16 *file_path,
                                                  EFI_STATUS *out_efi_status,
                                                  uint32_t *out_stage);

#endif
