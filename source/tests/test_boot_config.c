#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/boot_config.h>

int main(void) {
    static const char valid[] =
        "# XBoot entry\n"
        "title=Rescue Linux\n"
        "linux=/EFI/XBOOT/VMLINUZ\n"
        "initrd=/EFI/XBOOT/INITRD\n"
        "options=console=ttyS0 quiet\n"
        "measure=required\n"
        "secure_boot=optional\n"
        "timeout=5\n";
    static const char duplicate[] = "linux=/A\nlinux=/B\n";
    static const char unknown[] = "linux=/A\nunknown=value\n";
    static const char missing[] = "title=Only title\n";
    static const char invalid_policy[] = "linux=/A\nmeasure=mandatory\n";
    static const char duplicate_policy[] = "linux=/A\nmeasure=required\nmeasure=optional\n";
    xboot_boot_entry entry;

    assert(xboot_boot_config_parse((const uint8_t *)valid, sizeof(valid) - 1, &entry) == XBOOT_OK);
    assert(strcmp(entry.title, "Rescue Linux") == 0);
    assert(strcmp(entry.linux_path, "/EFI/XBOOT/VMLINUZ") == 0);
    assert(strcmp(entry.initrd_path, "/EFI/XBOOT/INITRD") == 0);
    assert(entry.timeout_seconds == 5 && entry.has_linux == 1);
    assert(entry.measure_required == 1 && entry.secure_boot_required == 0);
    assert(xboot_boot_config_parse((const uint8_t *)duplicate, sizeof(duplicate) - 1, &entry) == XBOOT_CORRUPT);
    assert(xboot_boot_config_parse((const uint8_t *)unknown, sizeof(unknown) - 1, &entry) == XBOOT_UNSUPPORTED);
    assert(xboot_boot_config_parse((const uint8_t *)missing, sizeof(missing) - 1, &entry) == XBOOT_NOT_FOUND);
    assert(xboot_boot_config_parse((const uint8_t *)invalid_policy, sizeof(invalid_policy) - 1, &entry) == XBOOT_CORRUPT);
    assert(xboot_boot_config_parse((const uint8_t *)duplicate_policy, sizeof(duplicate_policy) - 1, &entry) == XBOOT_CORRUPT);
    puts("boot config host tests: PASS");
    return 0;
}
