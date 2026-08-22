#ifndef XBOOT_UEFI_FS_H
#define XBOOT_UEFI_FS_H

#include <xboot/status.h>
#include <xboot/uefi.h>

xboot_status_t xboot_uefi_fs_open(EFI_BOOT_SERVICES *boot_services,
                                  EFI_HANDLE device_handle,
                                  EFI_FILE_PROTOCOL **out_root);

xboot_status_t xboot_uefi_fs_file_size(EFI_BOOT_SERVICES *boot_services,
                                       EFI_FILE_PROTOCOL *root,
                                       const CHAR16 *path,
                                       uint64_t *out_size);

xboot_status_t xboot_uefi_fs_read_file(EFI_BOOT_SERVICES *boot_services,
                                       EFI_FILE_PROTOCOL *root,
                                       const CHAR16 *path,
                                       void *buffer,
                                       size_t buffer_size,
                                       size_t *out_size);

#endif
