#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xboot/fat32.h>

static const uint8_t *image;
static size_t image_size;

static xboot_status_t read_blocks(void *context, uint32_t media_id, uint64_t lba,
                                  size_t buffer_size, void *buffer) {
    uint64_t offset;
    (void)context;
    (void)media_id;
    if (buffer == NULL || buffer_size == 0 || buffer_size % 512U != 0 ||
        lba > (image_size / 512U) - 1U || buffer_size / 512U > image_size / 512U - lba) {
        return XBOOT_INVALID_ARGUMENT;
    }
    offset = lba * 512ULL;
    memcpy(buffer, image + offset, buffer_size);
    return XBOOT_OK;
}

static void verify_file(const xboot_fat32_volume *volume, const char *path,
                        uint8_t *scratch, size_t scratch_size) {
    xboot_fat32_file file;
    uint8_t *output;
    size_t bytes = 0;
    assert(xboot_fat32_find(volume, path, scratch, scratch_size, &file) == XBOOT_OK);
    assert(file.size > 0 && file.size <= 128U * 1024U * 1024U);
    output = malloc(file.size);
    assert(output != NULL);
    assert(xboot_fat32_read_file(volume, &file, scratch, scratch_size,
                                 output, file.size, &bytes) == XBOOT_OK);
    assert(bytes == file.size);
    free(output);
}

int main(int argc, char **argv) {
    int fd;
    struct stat st;
    uint8_t boot_sector[512];
    uint8_t scratch[65536];
    xboot_block_device device;
    xboot_fat32_volume volume;
    if (argc != 2) return 2;
    fd = open(argv[1], O_RDONLY);
    assert(fd >= 0 && fstat(fd, &st) == 0 && st.st_size >= 512);
    image_size = (size_t)st.st_size;
    image = mmap(NULL, image_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(image != MAP_FAILED);
    device.context = NULL;
    device.read_blocks = read_blocks;
    device.media_id = 0;
    device.block_size = 512;
    device.last_block = image_size / 512U - 1U;
    assert(xboot_fat32_mount(&device, boot_sector, sizeof(boot_sector), &volume) == XBOOT_OK);
    verify_file(&volume, "EFI/BOOT/BOOTX64.EFI", scratch, sizeof(scratch));
    verify_file(&volume, "EFI/XBOOT/XBOOT~1.CON", scratch, sizeof(scratch));
    assert(xboot_fat32_find(&volume, "EFI/XBOOT/MISSING.BIN", scratch, sizeof(scratch),
                            &(xboot_fat32_file){0}) == XBOOT_NOT_FOUND);
    assert(munmap((void *)image, image_size) == 0);
    assert(close(fd) == 0);
    puts("real ESP FAT32 integration test: PASS");
    return 0;
}
