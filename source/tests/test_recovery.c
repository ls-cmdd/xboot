#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/recovery.h>

int main(void) {
    xboot_recovery_state state;
    assert(xboot_recovery_init(&state) == XBOOT_OK);
    assert(xboot_recovery_begin_boot(&state) == XBOOT_OK);
    assert(xboot_recovery_record(&state, XBOOT_IO_ERROR, "disk read failed") == XBOOT_OK);
    assert(xboot_recovery_mark_success(&state, 2) == XBOOT_OK);
    assert(state.boot_attempts == 1);
    assert(state.failed_attempts == 1);
    assert(state.last_successful_entry == 2);
    assert(xboot_recovery_event(&state, 0) != NULL);
    assert(strcmp(xboot_recovery_event(&state, 0)->message, "boot attempt succeeded") == 0);
    assert(strcmp(xboot_recovery_event(&state, 1)->message, "disk read failed") == 0);
    for (size_t i = 0; i < XBOOT_DIAGNOSTIC_CAPACITY + 3U; i++) {
        assert(xboot_recovery_record(&state, XBOOT_OK, "rotation") == XBOOT_OK);
    }
    assert(state.count == XBOOT_DIAGNOSTIC_CAPACITY);
    assert(xboot_recovery_event(&state, XBOOT_DIAGNOSTIC_CAPACITY) == NULL);
    puts("recovery diagnostics host tests: PASS");
    return 0;
}
