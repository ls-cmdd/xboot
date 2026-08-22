#include <xboot/security.h>

xboot_status_t xboot_security_check(const xboot_security_context *security,
                                    const uint8_t *image, size_t image_size,
                                    xboot_verify_image verify,
                                    xboot_measure_image measure,
                                    void *callback_context) {
    xboot_status_t status;
    if (security == NULL || image == NULL || image_size == 0) return XBOOT_INVALID_ARGUMENT;
    if (security->policy == XBOOT_SECURITY_PERMISSIVE) return XBOOT_OK;
    if (security->verifier_available == 0 || verify == NULL) return XBOOT_UNSUPPORTED;
    status = verify(callback_context, image, image_size);
    if (status != XBOOT_OK) return status == XBOOT_SECURITY_ERROR ? status : XBOOT_SECURITY_ERROR;
    if (security->policy == XBOOT_SECURITY_REQUIRE_VERIFICATION_AND_MEASUREMENT) {
        if (security->tpm_present == 0 || security->measurement_available == 0 || measure == NULL) {
            return XBOOT_UNSUPPORTED;
        }
        status = measure(callback_context, image, image_size);
        if (status != XBOOT_OK) return status == XBOOT_SECURITY_ERROR ? status : XBOOT_SECURITY_ERROR;
    }
    return XBOOT_OK;
}
