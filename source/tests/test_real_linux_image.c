#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xboot/linux_image.h>

int main(void) {
    FILE *file = fopen("assets/vmlinuz", "rb");
    long length;
    uint8_t *image;
    size_t image_size;
    xboot_linux_image_info info;
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    length = ftell(file);
    assert(length > 0);
    assert(fseek(file, 0, SEEK_SET) == 0);
    image_size = (size_t)length;
    image = malloc(image_size);
    assert(image != NULL);
    assert(fread(image, 1, image_size, file) == image_size);
    assert(fclose(file) == 0);
    assert(xboot_linux_inspect(image, image_size, &info) == XBOOT_OK);
    assert(info.boot_protocol >= 0x0208);
    assert(info.is_bzimage == 1);
    assert(info.protected_size != 0);
    assert(info.handover_offset != 0);
    assert((info.xloadflags & 0x0004U) != 0);
    assert((uint64_t)info.setup_size + info.protected_size <= image_size);
    assert((uint64_t)info.payload_offset + info.payload_length <=
           (uint64_t)info.setup_size + info.protected_size);
    printf("real Linux image: size=%zu protocol=0x%04x setup=%u protected=%u payload=%u+%u handover=0x%08x\n",
           image_size, info.boot_protocol, info.setup_size, info.protected_size,
           info.payload_offset, info.payload_length, info.handover_offset);
    free(image);
    puts("real Linux image inspector: PASS");
    return 0;
}
