#ifndef XBOOT_UEFI_LOADER_H
#define XBOOT_UEFI_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/memory.h>
#include <xboot/uefi.h>
#include <xboot/uefi_fs.h>

xboot_status_t xboot_uefi_load_file(EFI_BOOT_SERVICES *boot_services,
                                    EFI_FILE_PROTOCOL *root,
                                    const CHAR16 *path,
                                    xboot_memory_manager *memory,
                                    uint64_t *out_address,
                                    uint64_t *out_page_count,
                                    uint64_t *out_file_size);

#endif
