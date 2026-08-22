#include <xboot/uefi.h>
#include <xboot/handoff.h>
#include <xboot/memory.h>
#include <xboot/status.h>
#include <xboot/logger.h>
#include <xboot/resource.h>
#include <xboot/x86_cpu.h>
#include <xboot/acpi.h>
#include <xboot/smbios.h>
#include <xboot/uefi_block.h>
#include <xboot/gpt.h>
#include <xboot/uefi_fs.h>
#include <xboot/uefi_loader.h>
#include <xboot/linux_loader.h>
#include <xboot/boot_config.h>
#include <xboot/uefi_tcg2.h>
#include <xboot/uefi_linux.h>

extern xboot_status_t xboot_uefi_page_allocate(void *context,
                                                uint64_t page_count,
                                                uint64_t *out_address);
extern xboot_status_t xboot_uefi_page_allocate_below_4g(void *context,
                                                        uint64_t page_count,
                                                        uint64_t *out_address);
extern xboot_status_t xboot_uefi_page_free(void *context,
                                            uint64_t address,
                                            uint64_t page_count);

static EFI_SYSTEM_TABLE *g_system_table;
static EFI_BOOT_SERVICES *g_boot_services;
static EFI_HANDLE g_image_handle;
static xboot_logger g_logger;
static xboot_memory_manager g_memory_manager;
static xboot_resource_manager g_resource_manager;
static xboot_x86_cpu_info g_cpu_info;
static void *g_acpi_rsdp;
static uint8_t g_gpt_scratch[4096];
static xboot_gpt_summary g_gpt_summary;
static const CHAR16 g_config_path[] = {'\\','E','F','I','\\','X','B','O','O','T','\\','x','b','o','o','t','.','c','o','n','f',0};
static xboot_boot_entry g_boot_entry;
static CHAR16 g_kernel_path[XBOOT_CONFIG_STRING_MAX];
static CHAR16 g_initrd_path[XBOOT_CONFIG_STRING_MAX];
#ifndef XBOOT_DIRECT_LINUX
static CHAR16 g_linux_load_options[(XBOOT_CONFIG_STRING_MAX * 2U) + 32U];
#endif

static void log_line(const char *s);

void *memcpy(void *destination, const void *source, size_t size) {
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    for (size_t i = 0; i < size; i++) out[i] = in[i];
    return destination;
}

void *memset(void *destination, int value, size_t size) {
    uint8_t *out = (uint8_t *)destination;
    for (size_t i = 0; i < size; i++) out[i] = (uint8_t)value;
    return destination;
}

static void zero_memory(void *buffer, size_t size) {
    uint8_t *bytes = (uint8_t *)buffer;
    for (size_t i = 0; i < size; i++) bytes[i] = 0;
}

static xboot_status_t ascii_to_utf16(const char *source, CHAR16 *destination, size_t capacity) {
    size_t length = 0;
    if (source == NULL || destination == NULL || capacity == 0) return XBOOT_INVALID_ARGUMENT;
    while (source[length] != '\0') {
        if (length + 1U >= capacity || (uint8_t)source[length] >= 0x80U) return XBOOT_BUFFER_TOO_SMALL;
        length++;
    }
    for (size_t i = 0; i < length; i++) destination[i] = (CHAR16)(uint8_t)source[i];
    destination[length] = 0;
    return XBOOT_OK;
}

#ifndef XBOOT_DIRECT_LINUX
static xboot_status_t append_ascii(CHAR16 *destination, size_t capacity, size_t *length,
                                   const char *source) {
    size_t source_length = 0;
    if (destination == NULL || length == NULL || source == NULL) return XBOOT_INVALID_ARGUMENT;
    while (source[source_length] != '\0') source_length++;
    if (*length > capacity || source_length > capacity - *length - 1U) return XBOOT_BUFFER_TOO_SMALL;
    for (size_t i = 0; i < source_length; i++) destination[(*length)++] = (CHAR16)(uint8_t)source[i];
    destination[*length] = 0;
    return XBOOT_OK;
}

static xboot_status_t build_efi_load_options(const xboot_boot_entry *entry,
                                             CHAR16 *options, size_t capacity) {
    const char *base_options;
    size_t length = 0;
    xboot_status_t status;
    if (entry == NULL || options == NULL || capacity == 0) return XBOOT_INVALID_ARGUMENT;
    options[0] = 0;
    base_options = entry->options[0] == '\0' ? "console=ttyS0" : entry->options;
    status = append_ascii(options, capacity, &length, base_options);
    if (status != XBOOT_OK) return status;
    if (entry->initrd_path[0] != '\0') {
        status = append_ascii(options, capacity, &length, " initrd=");
        if (status != XBOOT_OK) return status;
        status = append_ascii(options, capacity, &length, entry->initrd_path);
    }
    return status;
}

#endif

static void default_boot_entry(xboot_boot_entry *entry) {
    static const char title[] = "Xenon Linux";
    static const char linux_path[] = "\\EFI\\XBOOT\\vmlinuz.efi";
    static const char initrd_path[] = "\\EFI\\XBOOT\\initrd.img";
    static const char options[] = "console=ttyS0";
    zero_memory(entry, sizeof(*entry));
    for (size_t i = 0; i < sizeof(title); i++) entry->title[i] = title[i];
    for (size_t i = 0; i < sizeof(linux_path); i++) entry->linux_path[i] = linux_path[i];
    for (size_t i = 0; i < sizeof(initrd_path); i++) entry->initrd_path[i] = initrd_path[i];
    for (size_t i = 0; i < sizeof(options); i++) entry->options[i] = options[i];
    entry->has_linux = 1;
}

static void uefi_log_sink(void *context, xboot_log_level level, const char *message) {
    (void)context;
    log_line("[CORE ");
    log_line(xboot_log_level_name(level));
    log_line("] ");
    log_line(message);
    log_line("\r\n");
}

static xboot_status_t release_page_resource(void *context,
                                             uint64_t address,
                                             uint64_t page_count) {
    return xboot_page_free((xboot_memory_manager *)context, address, page_count);
}

static xboot_status_t uefi_physical_read(void *context, uint64_t address,
                                         void *destination, size_t size) {
    const xboot_uefi_handoff *handoff = context;
    if (handoff == NULL || destination == NULL || size == 0 ||
        address > UINT64_MAX - size) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < handoff->memory_region_count; i++) {
        const xboot_memory_region *region = &handoff->memory_regions[i];
        uint64_t region_bytes;
        uint64_t region_end;
        if (region->page_count > UINT64_MAX / 4096ULL) {
            continue;
        }
        region_bytes = region->page_count * 4096ULL;
        if (region->physical_start > UINT64_MAX - region_bytes) {
            continue;
        }
        region_end = region->physical_start + region_bytes;
        if (address >= region->physical_start && address <= region_end &&
            address + size <= region_end) {
            volatile const uint8_t *source = (volatile const uint8_t *)(uintptr_t)address;
            uint8_t *target = destination;
            for (size_t j = 0; j < size; j++) {
                target[j] = source[j];
            }
            return XBOOT_OK;
        }
    }
    return XBOOT_IO_ERROR;
}

static void debugcon_putc(char c) {
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x402));
}

static void debugcon_write(const char *s) {
    while (*s != '\0') {
        debugcon_putc(*s++);
    }
}

static void console_write(const char *s) {
    CHAR16 buffer[256];
    size_t i = 0;
    if (g_system_table == NULL || g_system_table->ConOut == NULL ||
        g_system_table->ConOut->OutputString == NULL) {
        return;
    }
    while (s[i] != '\0' && i + 1 < sizeof(buffer) / sizeof(buffer[0])) {
        buffer[i] = (CHAR16)(uint8_t)s[i];
        i++;
    }
    buffer[i] = 0;
    (void)g_system_table->ConOut->OutputString(g_system_table->ConOut, buffer);
}

static void log_line(const char *s) {
    console_write(s);
    debugcon_write(s);
}

static void log_hex64(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    char buffer[19];
    unsigned int i;
    buffer[0] = '0';
    buffer[1] = 'x';
    for (i = 0; i < 16; i++) {
        buffer[2 + i] = digits[(value >> ((15 - i) * 4)) & 0xf];
    }
    buffer[18] = '\0';
    log_line(buffer);
}

static void log_u64(uint64_t value) {
    char reversed[21];
    char forward[21];
    unsigned int i = 0;
    unsigned int j;
    if (value == 0) {
        log_line("0");
        return;
    }
    while (value != 0 && i < sizeof(reversed) - 1) {
        reversed[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (j = 0; j < i; j++) {
        forward[j] = reversed[i - j - 1];
    }
    forward[i] = '\0';
    log_line(forward);
}

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

static void discover_configuration_tables(void) {
    EFI_GUID acpi20 = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID acpi10 = EFI_ACPI_TABLE_GUID;
    EFI_GUID smbios = SMBIOS_TABLE_GUID;
    EFI_GUID smbios3 = SMBIOS3_TABLE_GUID;
    BOOLEAN acpi_found = 0;
    BOOLEAN smbios_found = 0;
    UINTN i;

    log_line("[INFO] configuration tables: ");
    log_u64(g_system_table->NumberOfTableEntries);
    log_line("\r\n");

    for (i = 0; i < g_system_table->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *entry = &g_system_table->ConfigurationTable[i];
        if (guid_equal(&entry->VendorGuid, &acpi20) ||
            guid_equal(&entry->VendorGuid, &acpi10)) {
            acpi_found = 1;
            if (g_acpi_rsdp == NULL || guid_equal(&entry->VendorGuid, &acpi20)) {
                g_acpi_rsdp = entry->VendorTable;
            }
            log_line("[INFO] ACPI table: ");
            log_hex64((uint64_t)(uintptr_t)entry->VendorTable);
            log_line("\r\n");
        }
        if (guid_equal(&entry->VendorGuid, &smbios) ||
            guid_equal(&entry->VendorGuid, &smbios3)) {
            smbios_found = 1;
            log_line("[INFO] SMBIOS table: ");
            log_hex64((uint64_t)(uintptr_t)entry->VendorTable);
            log_line("\r\n");
        }
    }
    if (!acpi_found) {
        log_line("[WARN] ACPI configuration table unavailable\r\n");
    }
    if (!smbios_found) {
        log_line("[WARN] SMBIOS configuration table unavailable\r\n");
    }
}

static void discover_storage(void) {
    EFI_GUID loaded_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    xboot_block_device block_device;
    xboot_gpt_summary *gpt_summary = &g_gpt_summary;
    xboot_status_t status;
    if (g_boot_services == NULL || g_boot_services->HandleProtocol == NULL) return;
    if (g_boot_services->HandleProtocol(g_image_handle, &loaded_guid, (void **)&loaded_image) != EFI_SUCCESS ||
        loaded_image == NULL || loaded_image->DeviceHandle == NULL) {
        log_line("[WARN] loaded image device handle unavailable\r\n");
        return;
    }
    status = xboot_uefi_block_from_device(g_boot_services, loaded_image->DeviceHandle, &block_device);
    if (status != XBOOT_OK) {
        log_line("[WARN] boot device Block I/O unavailable: ");
        log_line(xboot_status_name(status));
        log_line("\r\n");
        return;
    }
    log_line("[INFO] UEFI Block I/O active, block size: ");
    log_u64(block_device.block_size);
    log_line("\r\n");
    if (block_device.block_size > sizeof(g_gpt_scratch)) {
        log_line("[WARN] GPT scan skipped: block size exceeds bounded scratch\r\n");
        return;
    }
    status = xboot_gpt_scan(&block_device, g_gpt_scratch, sizeof(g_gpt_scratch), gpt_summary);
    if (status == XBOOT_OK) {
        log_line("[INFO] GPT partitions discovered: ");
        log_u64(gpt_summary->active_partition_count);
        log_line("\r\n");
    } else if (status == XBOOT_NOT_FOUND) {
        log_line("[NOTICE] boot medium is not GPT or GPT header is unavailable\r\n");
    } else {
        log_line("[WARN] GPT validation failed: ");
        log_line(xboot_status_name(status));
        log_line("\r\n");
    }
}

static xboot_status_t load_boot_config(EFI_FILE_PROTOCOL *root,
                                         xboot_boot_entry *entry) {
    uint64_t address = 0;
    uint64_t pages = 0;
    uint64_t size = 0;
    xboot_status_t status;
    if (root == NULL || entry == NULL) return XBOOT_INVALID_ARGUMENT;
    status = xboot_uefi_load_file(g_boot_services, root, g_config_path, &g_memory_manager,
                                  &address, &pages, &size);
    if (status == XBOOT_OK) {
        if (size > SIZE_MAX) status = XBOOT_BUFFER_TOO_SMALL;
        else status = xboot_boot_config_parse((const uint8_t *)(uintptr_t)address,
                                              (size_t)size, entry);
    }
    if (address != 0) (void)xboot_page_free(&g_memory_manager, address, pages);
    return status;
}

static xboot_status_t load_boot_config_from_volume(xboot_boot_entry *entry) {
    EFI_GUID loaded_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_FILE_PROTOCOL *root = NULL;
    xboot_status_t status;
    if (entry == NULL || g_boot_services->HandleProtocol == NULL) return XBOOT_INVALID_ARGUMENT;
    status = (xboot_status_t)(g_boot_services->HandleProtocol(g_image_handle, &loaded_guid,
                                                               (void **)&loaded_image) == EFI_SUCCESS ? XBOOT_OK : XBOOT_NOT_FOUND);
    if (status != XBOOT_OK || loaded_image == NULL || loaded_image->DeviceHandle == NULL) return XBOOT_NOT_FOUND;
    status = xboot_uefi_fs_open(g_boot_services, loaded_image->DeviceHandle, &root);
    if (status == XBOOT_OK) status = load_boot_config(root, entry);
    if (root != NULL && root->Close != NULL) (void)root->Close(root);
    return status;
}

static xboot_status_t load_linux_artifacts(xboot_uefi_handoff *handoff,
                                             const xboot_boot_entry *entry,
                                             const CHAR16 *kernel_path,
                                             const CHAR16 *initrd_path,
                                             xboot_linux_boot_plan *out_plan) {
    EFI_GUID loaded_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_FILE_PROTOCOL *root = NULL;
    xboot_linux_boot_plan plan;
    uint64_t kernel_address = 0, kernel_pages = 0, kernel_size = 0;
    uint64_t initrd_address = 0, initrd_pages = 0, initrd_size = 0;
    xboot_status_t status;
    if (g_boot_services->HandleProtocol == NULL || handoff == NULL || entry == NULL ||
        kernel_path == NULL || initrd_path == NULL || out_plan == NULL) return XBOOT_INVALID_ARGUMENT;
    zero_memory(&plan, sizeof(plan));
    zero_memory(out_plan, sizeof(*out_plan));
    status = (xboot_status_t)(g_boot_services->HandleProtocol(g_image_handle, &loaded_guid, (void **)&loaded_image) == EFI_SUCCESS ? XBOOT_OK : XBOOT_NOT_FOUND);
    if (status != XBOOT_OK || loaded_image == NULL || loaded_image->DeviceHandle == NULL) return XBOOT_NOT_FOUND;
    status = xboot_uefi_fs_open(g_boot_services, loaded_image->DeviceHandle, &root);
    if (status != XBOOT_OK) return status;
    status = xboot_uefi_load_file(g_boot_services, root, kernel_path, &g_memory_manager,
                                  &kernel_address, &kernel_pages, &kernel_size);
    if (status != XBOOT_OK) {
        if (root->Close != NULL) (void)root->Close(root);
        return status;
    }
    if (initrd_path[0] != 0) {
        status = xboot_uefi_load_file(g_boot_services, root, initrd_path, &g_memory_manager,
                                      &initrd_address, &initrd_pages, &initrd_size);
        if (status != XBOOT_OK) {
            (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
            if (root->Close != NULL) (void)root->Close(root);
            return status;
        }
    }
    if (entry->has_kernel_sha256 != 0) {
        status = xboot_sha256_matches_hex((const uint8_t *)(uintptr_t)kernel_address,
                                          (size_t)kernel_size, entry->kernel_sha256);
        if (status != XBOOT_OK) {
            if (root->Close != NULL) (void)root->Close(root);
            if (initrd_address != 0) (void)xboot_page_free(&g_memory_manager, initrd_address, initrd_pages);
            (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
            return status;
        }
    }
    if (entry->has_initrd_sha256 != 0 && initrd_address == 0) {
        if (root->Close != NULL) (void)root->Close(root);
        (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
        return XBOOT_SECURITY_ERROR;
    }
    if (entry->has_initrd_sha256 != 0 && initrd_address != 0) {
        status = xboot_sha256_matches_hex((const uint8_t *)(uintptr_t)initrd_address,
                                          (size_t)initrd_size, entry->initrd_sha256);
        if (status != XBOOT_OK) {
            if (root->Close != NULL) (void)root->Close(root);
            if (initrd_address != 0) (void)xboot_page_free(&g_memory_manager, initrd_address, initrd_pages);
            (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
            return status;
        }
    }
    {
        xboot_status_t measure_status = xboot_uefi_tcg2_measure(g_boot_services,
                                                                 (const uint8_t *)(uintptr_t)kernel_address,
                                                                 (size_t)kernel_size, "XBoot Linux kernel");
        if (measure_status == XBOOT_OK) {
            log_line("[INFO] TCG2 measured Linux kernel into PCR4\r\n");
        } else if (entry->measure_required != 0) {
            log_line("[ERROR] TCG2 required kernel measurement failed: ");
            log_line(xboot_status_name(measure_status));
            log_line("\r\n");
            if (root->Close != NULL) (void)root->Close(root);
            if (initrd_address != 0) (void)xboot_page_free(&g_memory_manager, initrd_address, initrd_pages);
            (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
            return measure_status;
        } else if (measure_status == XBOOT_NOT_FOUND || measure_status == XBOOT_UNSUPPORTED) {
            log_line("[INFO] TCG2 measurement unavailable\r\n");
        } else {
            log_line("[WARN] TCG2 kernel measurement failed: ");
            log_line(xboot_status_name(measure_status));
            log_line("\r\n");
        }
        if (initrd_address != 0) {
            measure_status = xboot_uefi_tcg2_measure(g_boot_services,
                                                     (const uint8_t *)(uintptr_t)initrd_address,
                                                     (size_t)initrd_size, "XBoot Linux initrd");
            if (measure_status == XBOOT_OK) {
                log_line("[INFO] TCG2 measured Linux initrd into PCR4\r\n");
            } else if (entry->measure_required != 0) {
                log_line("[ERROR] TCG2 required initrd measurement failed: ");
                log_line(xboot_status_name(measure_status));
                log_line("\r\n");
                if (root->Close != NULL) (void)root->Close(root);
                (void)xboot_page_free(&g_memory_manager, initrd_address, initrd_pages);
                (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
                return measure_status;
            } else if (measure_status != XBOOT_NOT_FOUND && measure_status != XBOOT_UNSUPPORTED) {
                log_line("[WARN] TCG2 initrd measurement failed: ");
                log_line(xboot_status_name(measure_status));
                log_line("\r\n");
            }
        }
    }
    if (entry->has_kernel_sha256 != 0 || entry->has_initrd_sha256 != 0) {
        log_line("[INFO] Linux artifact hashes verified\r\n");
    } else {
        log_line("[INFO] Linux artifact hash policy not configured\r\n");
    }
    log_line("[INFO] Linux kernel/initramfs loaded, bytes: ");
    log_u64(kernel_size); log_line("/"); log_u64(initrd_size); log_line("\r\n");
    status = xboot_linux_load_plan((const uint8_t *)(uintptr_t)kernel_address, (size_t)kernel_size,
                                   (const uint8_t *)(uintptr_t)initrd_address, (size_t)initrd_size,
                                   entry->options[0] == '\0' ? "console=ttyS0" : entry->options,
                                   &g_memory_manager, &plan);
    if (status == XBOOT_OK) status = xboot_linux_build_boot_params((const uint8_t *)(uintptr_t)kernel_address,
                                                                     (size_t)kernel_size, handoff, &plan);
    if (root->Close != NULL) (void)root->Close(root);
    if (initrd_address != 0) (void)xboot_page_free(&g_memory_manager, initrd_address, initrd_pages);
    (void)xboot_page_free(&g_memory_manager, kernel_address, kernel_pages);
    if (status != XBOOT_OK) {
        if (plan.prepared != 0) (void)xboot_linux_release_plan(&g_memory_manager, &plan);
        return status;
    }
    log_line("[INFO] Linux boot_params built, e820 entries: ");
    log_u64(plan.e820_entries); log_line("\r\n");
    *out_plan = plan;
    return XBOOT_OK;
}

static xboot_status_t read_secure_boot_state(uint8_t *out_enabled) {
    static const CHAR16 secure_boot_name[] = {'S','e','c','u','r','e','B','o','o','t',0};
    EFI_GUID global_guid = EFI_GLOBAL_VARIABLE_GUID;
    uint32_t attributes = 0;
    UINTN data_size = 1;
    uint8_t value = 0;
    EFI_STATUS firmware_status;
    if (out_enabled == NULL || g_system_table == NULL || g_system_table->RuntimeServices == NULL ||
        g_system_table->RuntimeServices->GetVariable == NULL) return XBOOT_UNSUPPORTED;
    firmware_status = g_system_table->RuntimeServices->GetVariable((CHAR16 *)secure_boot_name,
                                                                     &global_guid, &attributes,
                                                                     &data_size, &value);
    if (firmware_status == EFI_NOT_FOUND) {
        *out_enabled = 0;
        return XBOOT_OK;
    }
    if (firmware_status != EFI_SUCCESS) {
        log_line("[ERROR] GetVariable(SecureBoot) EFI_STATUS=");
        log_hex64(firmware_status);
        log_line(" size=");
        log_u64(data_size);
        log_line("\r\n");
        return XBOOT_PROTOCOL_ERROR;
    }
    if (data_size != 1U) return XBOOT_CORRUPT;
    *out_enabled = value != 0 ? 1U : 0U;
    return XBOOT_OK;
}

static void discover_cpu(void) {
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];
    uint32_t max_leaf;

    __asm__ volatile("cpuid" : "=a"(max_leaf), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    vendor[0] = (char)(ebx & 0xff); vendor[1] = (char)((ebx >> 8) & 0xff);
    vendor[2] = (char)((ebx >> 16) & 0xff); vendor[3] = (char)((ebx >> 24) & 0xff);
    vendor[4] = (char)(edx & 0xff); vendor[5] = (char)((edx >> 8) & 0xff);
    vendor[6] = (char)((edx >> 16) & 0xff); vendor[7] = (char)((edx >> 24) & 0xff);
    vendor[8] = (char)(ecx & 0xff); vendor[9] = (char)((ecx >> 8) & 0xff);
    vendor[10] = (char)((ecx >> 16) & 0xff); vendor[11] = (char)((ecx >> 24) & 0xff);
    vendor[12] = '\0';
    log_line("[INFO] CPU vendor: "); log_line(vendor); log_line("\r\n");

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    log_line("[INFO] CPU family/model leaf: "); log_hex64(eax); log_line("\r\n");
    log_line("[INFO] CPU features ECX: "); log_hex64(ecx); log_line("\r\n");
    log_line("[INFO] CPU features EDX: "); log_hex64(edx); log_line("\r\n");
    if (max_leaf >= 7) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
        log_line("[INFO] extended CPU features EBX: "); log_hex64(ebx); log_line("\r\n");
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    xboot_status_t config_status;
    xboot_status_t path_status;
    g_image_handle = image_handle;
    g_system_table = system_table;
    if (system_table == NULL || system_table->BootServices == NULL ||
        system_table->Hdr.Signature != EFI_SYSTEM_TABLE_SIGNATURE) {
        g_boot_services = NULL;
        debugcon_write("[FATAL] invalid EFI system table\r\n");
        return 1;
    }
    g_boot_services = system_table->BootServices;

    (void)g_boot_services->SetWatchdogTimer(0, 0, 0, NULL);
    log_line("XBoot phase 1 - UEFI x86_64 entry\r\n");
    log_line("[INFO] EFI system table validated\r\n");
    if (xboot_logger_init(&g_logger, 1, XBOOT_LOG_INFO) != XBOOT_OK ||
        xboot_logger_add_sink(&g_logger, uefi_log_sink, NULL, XBOOT_LOG_INFO) != XBOOT_OK) {
        log_line("[FATAL] unified logger initialization failed\r\n");
        return 1;
    }
    xboot_logger_write(&g_logger, XBOOT_LOG_NOTICE, "unified logger active");
    discover_cpu();
    {
        xboot_status_t cpu_status = xboot_x86_cpu_discover(&g_cpu_info);
        if (cpu_status != XBOOT_OK) {
            log_line("[ERROR] extended x86 CPU discovery failed: ");
            log_line(xboot_status_name(cpu_status));
            log_line("\r\n");
            return 1;
        }
        log_line("[INFO] physical address bits: ");
        log_u64(g_cpu_info.physical_address_bits);
        log_line("\r\n");
        log_line("[INFO] virtual address bits: ");
        log_u64(g_cpu_info.virtual_address_bits);
        log_line("\r\n");
        log_line("[INFO] topology logical processors: ");
        log_u64(g_cpu_info.logical_processors);
        log_line("\r\n");
        log_line("[INFO] x86 NX/SMEP/SMAP/APIC/x2APIC: ");
        log_u64(g_cpu_info.has_nx);
        log_u64(g_cpu_info.has_smep);
        log_u64(g_cpu_info.has_smap);
        log_u64(g_cpu_info.has_apic);
        log_u64(g_cpu_info.has_x2apic);
        log_line("\r\n");
    }
    discover_configuration_tables();
    discover_storage();
    {
        xboot_uefi_handoff *handoff = NULL;
        xboot_page_backend page_backend = {
            .context = g_boot_services,
#ifdef XBOOT_DIRECT_LINUX
            .allocate_pages = xboot_uefi_page_allocate_below_4g,
#else
            .allocate_pages = xboot_uefi_page_allocate,
#endif
            .free_pages = xboot_uefi_page_free,
        };
        uint64_t test_page = 0;
        xboot_status_t xboot_status;

        xboot_status = xboot_uefi_collect_handoff(system_table, g_boot_services,
                                                   &handoff);
        if (xboot_status != XBOOT_OK) {
            log_line("[ERROR] UEFI handoff collection failed: ");
            log_line(xboot_status_name(xboot_status));
            log_line("\r\n");
            return 1;
        }
        log_line("[INFO] handoff memory regions: ");
        log_u64(handoff->memory_region_count);
        log_line("\r\n");
        log_line("[INFO] handoff map key: ");
        log_u64(handoff->uefi_map_key);
        log_line("\r\n");
        if (handoff->acpi_table != NULL) {
            xboot_acpi_source acpi_source = {
                .context = handoff,
                .read = uefi_physical_read,
            };
            xboot_acpi_summary acpi_summary;
            xboot_status = xboot_acpi_parse(&acpi_source,
                                            (uint64_t)(uintptr_t)handoff->acpi_table,
                                            &acpi_summary);
            if (xboot_status != XBOOT_OK) {
                log_line("[ERROR] ACPI parser rejected firmware tables: ");
                log_line(xboot_status_name(xboot_status));
                log_line("\r\n");
                xboot_uefi_release_handoff(g_boot_services, handoff);
                return 1;
            }
            log_line("[INFO] ACPI tables parsed: ");
            log_u64(acpi_summary.table_count);
            log_line("\r\n");
            log_line("[INFO] ACPI MADT/MCFG/FADT: ");
            log_u64(acpi_summary.madt_count);
            log_u64(acpi_summary.mcfg_count);
            log_u64(acpi_summary.fadt_count);
            log_line("\r\n");
        }
        if (handoff->smbios_table != NULL) {
            xboot_smbios_source smbios_source = {
                .context = handoff,
                .read = uefi_physical_read,
            };
            xboot_smbios_summary smbios_summary;
            xboot_status = xboot_smbios_parse(&smbios_source,
                                              (uint64_t)(uintptr_t)handoff->smbios_table,
                                              &smbios_summary);
            if (xboot_status != XBOOT_OK) {
                log_line("[WARN] SMBIOS parser rejected firmware table: ");
                log_line(xboot_status_name(xboot_status));
                log_line("\r\n");
            } else {
                log_line("[INFO] SMBIOS structures parsed: ");
                log_u64(smbios_summary.structure_count);
                log_line("\r\n");
                log_line("[INFO] SMBIOS BIOS/system/processor/memory: ");
                log_u64(smbios_summary.type0_count);
                log_u64(smbios_summary.type1_count);
                log_u64(smbios_summary.type4_count);
                log_u64(smbios_summary.type17_count);
                log_line("\r\n");
            }
        }

        xboot_status = xboot_memory_manager_init(&g_memory_manager, page_backend);
        if (xboot_status == XBOOT_OK) {
            xboot_status = xboot_page_alloc(&g_memory_manager, 1, &test_page);
        }
        if (xboot_status == XBOOT_OK) {
            log_line("[INFO] page allocator acquired page: ");
            log_hex64(test_page);
            log_line("\r\n");
            xboot_status = xboot_resource_manager_init(&g_resource_manager);
        }
        if (xboot_status == XBOOT_OK) {
            uint64_t resource_id = 0;
            xboot_status = xboot_resource_acquire(&g_resource_manager,
                                                  XBOOT_RESOURCE_MEMORY,
                                                  "uefi-page-test", test_page, 1,
                                                  &g_memory_manager,
                                                  release_page_resource,
                                                  &resource_id);
            if (xboot_status == XBOOT_OK) {
                xboot_status = xboot_resource_release(&g_resource_manager, resource_id);
            }
        }
        if (xboot_status != XBOOT_OK ||
            xboot_page_allocation_count(&g_memory_manager) != 0 ||
            xboot_resource_active_count(&g_resource_manager) != 0) {
            log_line("[ERROR] page/resource lifecycle failed: ");
            log_line(xboot_status_name(xboot_status));
            log_line("\r\n");
            xboot_uefi_release_handoff(g_boot_services, handoff);
            return 1;
        }
        log_line("[INFO] page allocator lifecycle verified\r\n");
        log_line("[INFO] resource lifecycle verified\r\n");
        default_boot_entry(&g_boot_entry);
        config_status = load_boot_config_from_volume(&g_boot_entry);
        if (config_status == XBOOT_OK) {
            log_line("[INFO] xboot.conf loaded\r\n");
        } else if (config_status == XBOOT_NOT_FOUND) {
            log_line("[WARN] xboot.conf not found; using built-in safe entry\r\n");
        } else {
            log_line("[ERROR] xboot.conf rejected: ");
            log_line(xboot_status_name(config_status));
            log_line("\r\n");
            xboot_uefi_release_handoff(g_boot_services, handoff);
            return 1;
        }
        path_status = ascii_to_utf16(g_boot_entry.linux_path, g_kernel_path,
                                     sizeof(g_kernel_path) / sizeof(g_kernel_path[0]));
        if (path_status == XBOOT_OK) {
            path_status = ascii_to_utf16(g_boot_entry.initrd_path, g_initrd_path,
                                         sizeof(g_initrd_path) / sizeof(g_initrd_path[0]));
        }
#ifndef XBOOT_DIRECT_LINUX
        if (path_status == XBOOT_OK) {
            path_status = build_efi_load_options(&g_boot_entry, g_linux_load_options,
                                                  sizeof(g_linux_load_options) / sizeof(g_linux_load_options[0]));
        }
#endif
        if (path_status != XBOOT_OK || g_boot_entry.has_linux == 0) {
            log_line("[ERROR] xboot.conf entry is unusable\r\n");
            xboot_uefi_release_handoff(g_boot_services, handoff);
            return 1;
        }
        xboot_linux_boot_plan linux_plan;
        zero_memory(&linux_plan, sizeof(linux_plan));
        xboot_status = load_linux_artifacts(handoff, &g_boot_entry, g_kernel_path, g_initrd_path, &linux_plan);
        if (xboot_status == XBOOT_OK) {
#ifdef XBOOT_DIRECT_LINUX
            {
                uint8_t secure_boot_enabled = 0;
                EFI_STATUS auth_efi_status = EFI_SUCCESS;
                uint32_t auth_stage = 0;
                xboot_status = read_secure_boot_state(&secure_boot_enabled);
                if (xboot_status != XBOOT_OK) {
                    log_line("[ERROR] Secure Boot state unavailable\r\n");
                    (void)xboot_linux_release_plan(&g_memory_manager, &linux_plan);
                    xboot_uefi_release_handoff(g_boot_services, handoff);
                    return 1;
                }
                if (secure_boot_enabled != 0) {
                    log_line("[INFO] Secure Boot enabled; authenticating Linux image through UEFI\r\n");
                    xboot_status = xboot_uefi_authenticate_linux_file(g_boot_services, g_image_handle,
                                                                       g_kernel_path, &auth_efi_status,
                                                                       &auth_stage);
                    if (xboot_status != XBOOT_OK) {
                        log_line("[ERROR] Secure Boot image authentication failed: ");
                        log_line(xboot_status_name(xboot_status));
                        log_line(" EFI_STATUS=");
                        log_hex64(auth_efi_status);
                        log_line(" stage=");
                        log_u64(auth_stage);
                        log_line("\r\n");
                        (void)xboot_linux_release_plan(&g_memory_manager, &linux_plan);
                        xboot_uefi_release_handoff(g_boot_services, handoff);
                        return 1;
                    }
                    log_line("[INFO] Secure Boot firmware authentication passed\r\n");
                }
            }
            log_line("[NOTICE] Starting Linux through direct 64-bit boot protocol\r\n");
            xboot_status = xboot_linux_direct_start(g_image_handle, g_boot_services, handoff,
                                                    &g_memory_manager, &linux_plan);
            log_line("[ERROR] direct Linux entry returned: ");
            log_line(xboot_status_name(xboot_status));
            log_line("\r\n");
            (void)xboot_linux_release_plan(&g_memory_manager, &linux_plan);
            xboot_uefi_release_handoff(g_boot_services, handoff);
            return 1;
#else
            EFI_HANDLE linux_image = NULL;
            EFI_STATUS linux_efi_status = EFI_SUCCESS;
            uint32_t linux_stage = 0;
            if (g_boot_entry.secure_boot_required != 0) {
                uint8_t secure_boot_enabled = 0;
                xboot_status_t secure_boot_status = read_secure_boot_state(&secure_boot_enabled);
                if (secure_boot_status != XBOOT_OK || secure_boot_enabled == 0) {
                    log_line("[ERROR] secure_boot=required but Secure Boot is not enabled\r\n");
                    (void)xboot_linux_release_plan(&g_memory_manager, &linux_plan);
                    xboot_uefi_release_handoff(g_boot_services, handoff);
                    return 1;
                }
            }
            log_line("[INFO] Linux artifact validation completed\r\n");
            log_line("[NOTICE] Starting Linux via UEFI PE/COFF EFI stub\r\n");
            xboot_status = xboot_uefi_start_linux_file(g_boot_services, g_image_handle,
                                                       g_kernel_path,
                                                       g_linux_load_options,
                                                       (UINTN)sizeof(g_linux_load_options),
                                                       &linux_image, &linux_efi_status, &linux_stage);
            if (xboot_status != XBOOT_OK) {
                log_line("[WARN] Linux EFI stub returned: ");
                log_line(xboot_status_name(xboot_status));
                log_line(" EFI_STATUS=");
                log_hex64(linux_efi_status);
                log_line(" stage=");
                log_u64(linux_stage);
                log_line("\r\n");
            } else {
                log_line("[NOTICE] Linux EFI image returned control to XBoot\r\n");
            }
            (void)xboot_linux_release_plan(&g_memory_manager, &linux_plan);
#endif
        } else if (xboot_status == XBOOT_NOT_FOUND && g_boot_entry.measure_required == 0) {
            log_line("[NOTICE] Linux artifacts not present on EFI volume\r\n");
        } else {
            log_line("[WARN] Linux artifact validation failed: ");
            log_line(xboot_status_name(xboot_status));
            log_line("\r\n");
        }
        xboot_uefi_release_handoff(g_boot_services, handoff);
    }
    log_line("[NOTICE] ExitBootServices handoff remains reserved for the Linux-loader phase\r\n");
    log_line("[NOTICE] XBoot phase 1 completed without claiming unsupported features\r\n");
    (void)g_boot_services->Stall(2500000);
    return EFI_SUCCESS;
}
