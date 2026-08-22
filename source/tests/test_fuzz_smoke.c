#include <stdint.h>
#include <stdio.h>
#include <xboot/boot_config.h>
#include <xboot/linux_image.h>

static uint32_t next_random(uint32_t *state) {
    *state = *state * 1664525U + 1013904223U;
    return *state;
}

int main(void) {
    uint8_t buffer[8192];
    uint32_t state = 0x58424f54U;
    for (uint32_t iteration = 0; iteration < 100000U; iteration++) {
        size_t length = (size_t)(next_random(&state) % (sizeof(buffer) + 1U));
        xboot_boot_entry entry;
        xboot_linux_image_info image;
        for (size_t i = 0; i < length; i++) buffer[i] = (uint8_t)next_random(&state);
        (void)xboot_boot_config_parse(buffer, length, &entry);
        (void)xboot_linux_inspect(buffer, length, &image);
    }
    puts("parser fuzz smoke: PASS (100000 deterministic inputs)");
    return 0;
}
