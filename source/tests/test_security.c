#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xboot/security.h>

static xboot_status_t verify_ok(void *context, const uint8_t *image, size_t size) {
    (void)context; (void)image; (void)size; return XBOOT_OK;
}

static xboot_status_t verify_bad(void *context, const uint8_t *image, size_t size) {
    (void)context; (void)image; (void)size; return XBOOT_SECURITY_ERROR;
}

static xboot_status_t measure_ok(void *context, const uint8_t *image, size_t size) {
    (void)context; (void)image; (void)size; return XBOOT_OK;
}

int main(void) {
    static const uint8_t image[] = {1, 2, 3};
    xboot_security_context context = {0};
    context.policy = XBOOT_SECURITY_PERMISSIVE;
    assert(xboot_security_check(&context, image, sizeof(image), NULL, NULL, NULL) == XBOOT_OK);
    context.policy = XBOOT_SECURITY_REQUIRE_VERIFICATION;
    assert(xboot_security_check(&context, image, sizeof(image), NULL, NULL, NULL) == XBOOT_UNSUPPORTED);
    context.verifier_available = 1;
    assert(xboot_security_check(&context, image, sizeof(image), verify_ok, NULL, NULL) == XBOOT_OK);
    assert(xboot_security_check(&context, image, sizeof(image), verify_bad, NULL, NULL) == XBOOT_SECURITY_ERROR);
    context.policy = XBOOT_SECURITY_REQUIRE_VERIFICATION_AND_MEASUREMENT;
    assert(xboot_security_check(&context, image, sizeof(image), verify_ok, measure_ok, NULL) == XBOOT_UNSUPPORTED);
    context.tpm_present = 1;
    context.measurement_available = 1;
    assert(xboot_security_check(&context, image, sizeof(image), verify_ok, measure_ok, NULL) == XBOOT_OK);
    puts("security policy host tests: PASS");
    return 0;
}
