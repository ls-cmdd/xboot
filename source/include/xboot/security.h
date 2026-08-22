#ifndef XBOOT_SECURITY_H
#define XBOOT_SECURITY_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef enum {
    XBOOT_SECURITY_PERMISSIVE = 0,
    XBOOT_SECURITY_REQUIRE_VERIFICATION = 1,
    XBOOT_SECURITY_REQUIRE_VERIFICATION_AND_MEASUREMENT = 2,
} xboot_security_policy;

typedef struct {
    uint8_t secure_boot_enabled;
    uint8_t tpm_present;
    uint8_t verifier_available;
    uint8_t measurement_available;
    xboot_security_policy policy;
} xboot_security_context;

typedef xboot_status_t (*xboot_verify_image)(void *context,
                                             const uint8_t *image,
                                             size_t image_size);
typedef xboot_status_t (*xboot_measure_image)(void *context,
                                              const uint8_t *image,
                                              size_t image_size);

xboot_status_t xboot_security_check(const xboot_security_context *security,
                                    const uint8_t *image,
                                    size_t image_size,
                                    xboot_verify_image verify,
                                    xboot_measure_image measure,
                                    void *callback_context);

#endif
