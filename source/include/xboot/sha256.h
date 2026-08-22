#ifndef XBOOT_SHA256_H
#define XBOOT_SHA256_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

#define XBOOT_SHA256_DIGEST_SIZE 32U
#define XBOOT_SHA256_HEX_SIZE 65U

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[64];
    size_t buffer_size;
} xboot_sha256_context;

void xboot_sha256_init(xboot_sha256_context *context);
void xboot_sha256_update(xboot_sha256_context *context, const uint8_t *data, size_t size);
void xboot_sha256_final(xboot_sha256_context *context, uint8_t digest[XBOOT_SHA256_DIGEST_SIZE]);
xboot_status_t xboot_sha256_hex(const uint8_t *data, size_t size,
                                char output[XBOOT_SHA256_HEX_SIZE]);
xboot_status_t xboot_sha256_matches_hex(const uint8_t *data, size_t size,
                                        const char expected[XBOOT_SHA256_HEX_SIZE]);

#endif
