#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xboot/boot_config.h>

static void parse_config(const char *path, int expect_hash) {
    FILE *file = fopen(path, "rb");
    long length;
    uint8_t *text;
    xboot_boot_entry entry;
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    length = ftell(file);
    assert(length > 0 && length < 4096);
    assert(fseek(file, 0, SEEK_SET) == 0);
    text = (uint8_t *)malloc((size_t)length);
    assert(text != NULL);
    assert(fread(text, 1, (size_t)length, file) == (size_t)length);
    assert(fclose(file) == 0);
    assert(xboot_boot_config_parse(text, (size_t)length, &entry) == XBOOT_OK);
    assert(entry.has_linux == 1);
    assert(strcmp(entry.linux_path, "\\EFI\\XBOOT\\vmlinuz.efi") == 0);
    assert(strcmp(entry.initrd_path, "\\EFI\\XBOOT\\initrd.img") == 0);
    assert(strcmp(entry.options, "console=ttyS0") == 0);
    assert(entry.has_kernel_sha256 == (uint8_t)expect_hash);
    assert(entry.has_initrd_sha256 == (uint8_t)expect_hash);
    if (expect_hash) {
        assert(strlen(entry.kernel_sha256) == 64);
        assert(strlen(entry.initrd_sha256) == 64);
    }
    free(text);
}

int main(void) {
    parse_config("config/xboot.conf", 0);
    parse_config("config/xboot-hash.conf", 1);
    puts("real xboot.conf integration test: PASS");
    return 0;
}
