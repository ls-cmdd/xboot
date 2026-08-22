#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/boot_manager.h>

int main(void) {
    xboot_boot_manager manager;
    xboot_boot_entry good = {0};
    xboot_boot_entry bad = {0};
    good.has_linux = 1;
    strcpy(good.title, "Linux");
    strcpy(good.linux_path, "/EFI/XBOOT/VMLINUZ");
    bad.has_linux = 1;
    strcpy(bad.linux_path, "relative/path");
    assert(xboot_boot_manager_init(&manager) == XBOOT_OK);
    assert(xboot_boot_manager_add(&manager, &bad) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_boot_manager_add(&manager, &good) == XBOOT_OK);
    assert(xboot_boot_manager_select_first_valid(&manager) == XBOOT_OK);
    assert(xboot_boot_manager_selected(&manager) != NULL);
    assert(strcmp(xboot_boot_manager_selected(&manager)->linux_path,
                  "/EFI/XBOOT/VMLINUZ") == 0);
    assert(xboot_boot_manager_select(&manager, 2) == XBOOT_INVALID_ARGUMENT);
    puts("boot manager host tests: PASS");
    return 0;
}
