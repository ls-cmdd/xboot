#include <assert.h>
#include <stdio.h>
#include <xboot/x86_cpu.h>

int main(void) {
    xboot_x86_cpu_info info;
    assert(xboot_x86_cpu_discover(NULL) == XBOOT_INVALID_ARGUMENT);
    assert(xboot_x86_cpu_discover(&info) == XBOOT_OK);
    assert(info.vendor[0] != '\0');
    assert(info.max_basic_leaf >= 1);
    assert(info.physical_address_bits >= 32);
    assert(info.virtual_address_bits >= 32);
    assert(info.has_tsc == 1);
    puts("x86 cpu discovery host tests: PASS");
    return 0;
}
