#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <xboot/linux_loader.h>

static uint8_t image[0x3000];
typedef struct { void *address; size_t size; } mapping_record;
static mapping_record mappings[8];

static void put16(uint8_t *p, uint16_t value) { p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t *p, uint32_t value) { p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24); }
static void put64(uint8_t *p, uint64_t value) { put32(p, (uint32_t)value); put32(p + 4, (uint32_t)(value >> 32)); }
static void make_image(void) {
    memset(image, 0, sizeof(image));
    image[0x1f1] = 4; put16(image + 0x1fe, 0xaa55); memcpy(image + 0x202, "HdrS", 4);
    put16(image + 0x206, 0x0215); put32(image + 0x1f4, 0x200); image[0x211] = 1; put32(image + 0x22c, 0x37ffffff);
    put32(image + 0x230, 0x1000); image[0x234] = 1; put16(image + 0x236, 1);
    put32(image + 0x238, 4096); put32(image + 0x248, 0x1000); put32(image + 0x24c, 0x200);
    put64(image + 0x258, 0x100000); put32(image + 0x260, 0x400000);
    for (size_t i = 0; i < 0x2000; i++) image[0x0A00 + i] = (uint8_t)(i ^ 0x5aU);
}
static xboot_status_t alloc_pages(void *context, uint64_t pages, uint64_t *address) {
    (void)context;
    size_t size = (size_t)pages * 4096U;
    void *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (mapping == MAP_FAILED) return XBOOT_NO_MEMORY;
    for (size_t i = 0; i < 8; i++) if (mappings[i].address == NULL) { mappings[i].address = mapping; mappings[i].size = size; break; }
    *address = (uint64_t)(uintptr_t)mapping;
    return XBOOT_OK;
}
static xboot_status_t free_pages(void *context, uint64_t address, uint64_t pages) {
    (void)context; (void)pages;
    for (size_t i = 0; i < 8; i++) if ((uint64_t)(uintptr_t)mappings[i].address == address) {
        assert(munmap(mappings[i].address, mappings[i].size) == 0); mappings[i].address = NULL; mappings[i].size = 0; return XBOOT_OK;
    }
    return XBOOT_NOT_FOUND;
}
int main(void) {
    xboot_page_backend backend = {0, alloc_pages, free_pages};
    xboot_memory_manager memory;
    xboot_linux_boot_plan plan;
    const char cmdline[] = "console=ttyS0 root=/dev/ram0";
    make_image();
    assert(xboot_memory_manager_init(&memory, backend) == XBOOT_OK);
    xboot_memory_region regions[2] = {{EFI_CONVENTIONAL_MEMORY, 0x100000, 0, 16, 0}, {5, 0x200000, 0, 4, 0}};
    xboot_uefi_handoff handoff = {regions, 2, 0, 0, 0, NULL, NULL, 0, 0, NULL, 0};
    assert(xboot_linux_load_plan(image, sizeof(image), NULL, 0, cmdline, &memory, &plan) == XBOOT_OK);
    assert(plan.prepared == 1 && plan.kernel_size == 0x400000 && plan.kernel_address != 0);
    assert(((uint8_t *)(uintptr_t)plan.kernel_address)[0] == (uint8_t)0x5a);
    assert(plan.cmdline_address != 0 && strcmp((const char *)(uintptr_t)plan.cmdline_address, cmdline) == 0);
    assert(xboot_linux_build_boot_params(image, sizeof(image), &handoff, &plan) == XBOOT_OK);
    assert(((uint8_t *)(uintptr_t)plan.boot_params_address)[0x210] == 0xFF);
    assert(((uint8_t *)(uintptr_t)plan.boot_params_address)[0x1E8] == 2);
    assert(((uint8_t *)(uintptr_t)plan.boot_params_address)[0x2D0 + 16] == 1);
    assert(xboot_linux_release_plan(&memory, &plan) == XBOOT_OK);
    assert(xboot_page_allocation_count(&memory) == 0);
    puts("Linux page-backed loader host tests: PASS");
    return 0;
}
