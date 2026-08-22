#include <assert.h>
#include <stdio.h>
#include <xboot/architecture.h>

int main(void) {
    xboot_architecture_info info;
    assert(xboot_architecture_current(&info) == XBOOT_OK);
    assert(info.architecture == XBOOT_ARCH_X86_64);
    assert(info.platform == XBOOT_PLATFORM_UEFI);
    assert(xboot_architecture_query(XBOOT_ARCH_X86_64, XBOOT_PLATFORM_UEFI, &info) == XBOOT_OK);
    assert(info.implementation_available == 1);
    assert(xboot_architecture_query(XBOOT_ARCH_X86_64, XBOOT_PLATFORM_BIOS, &info) == XBOOT_UNSUPPORTED);
    assert(xboot_architecture_query(XBOOT_ARCH_ARM64, XBOOT_PLATFORM_UEFI, &info) == XBOOT_UNSUPPORTED);
    assert(xboot_architecture_query(XBOOT_ARCH_RISCV64, XBOOT_PLATFORM_UEFI, &info) == XBOOT_UNSUPPORTED);
    puts("architecture boundary host tests: PASS");
    return 0;
}
