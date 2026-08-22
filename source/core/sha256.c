#include <xboot/sha256.h>

static const uint32_t k[64] = {
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static uint32_t rotate_right(uint32_t value, uint32_t amount) {
    return (value >> amount) | (value << (32U - amount));
}

static uint32_t read_be32(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static void write_be32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void transform(xboot_sha256_context *context, const uint8_t block[64]) {
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;
    for (size_t i = 0; i < 16; i++) words[i] = read_be32(block + i * 4U);
    for (size_t i = 16; i < 64; i++) {
        uint32_t s0 = rotate_right(words[i - 15U], 7U) ^ rotate_right(words[i - 15U], 18U) ^ (words[i - 15U] >> 3U);
        uint32_t s1 = rotate_right(words[i - 2U], 17U) ^ rotate_right(words[i - 2U], 19U) ^ (words[i - 2U] >> 10U);
        words[i] = words[i - 16U] + s0 + words[i - 7U] + s1;
    }
    a = context->state[0]; b = context->state[1]; c = context->state[2]; d = context->state[3];
    e = context->state[4]; f = context->state[5]; g = context->state[6]; h = context->state[7];
    for (size_t i = 0; i < 64; i++) {
        uint32_t s1 = rotate_right(e, 6U) ^ rotate_right(e, 11U) ^ rotate_right(e, 25U);
        uint32_t choice = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + choice + k[i] + words[i];
        uint32_t s0 = rotate_right(a, 2U) ^ rotate_right(a, 13U) ^ rotate_right(a, 22U);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + majority;
        h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
    }
    context->state[0] += a; context->state[1] += b; context->state[2] += c; context->state[3] += d;
    context->state[4] += e; context->state[5] += f; context->state[6] += g; context->state[7] += h;
}

void xboot_sha256_init(xboot_sha256_context *context) {
    if (context == NULL) return;
    context->state[0] = 0x6a09e667U; context->state[1] = 0xbb67ae85U;
    context->state[2] = 0x3c6ef372U; context->state[3] = 0xa54ff53aU;
    context->state[4] = 0x510e527fU; context->state[5] = 0x9b05688cU;
    context->state[6] = 0x1f83d9abU; context->state[7] = 0x5be0cd19U;
    context->bit_count = 0;
    context->buffer_size = 0;
}

void xboot_sha256_update(xboot_sha256_context *context, const uint8_t *data, size_t size) {
    if (context == NULL || (data == NULL && size != 0)) return;
    context->bit_count += (uint64_t)size * 8U;
    while (size != 0) {
        size_t available = 64U - context->buffer_size;
        size_t amount = size < available ? size : available;
        for (size_t i = 0; i < amount; i++) context->buffer[context->buffer_size + i] = data[i];
        context->buffer_size += amount;
        data += amount;
        size -= amount;
        if (context->buffer_size == 64U) {
            transform(context, context->buffer);
            context->buffer_size = 0;
        }
    }
}

void xboot_sha256_final(xboot_sha256_context *context, uint8_t digest[XBOOT_SHA256_DIGEST_SIZE]) {
    uint64_t bit_count;
    if (context == NULL || digest == NULL) return;
    bit_count = context->bit_count;
    context->buffer[context->buffer_size++] = 0x80U;
    while (context->buffer_size != 56U) {
        if (context->buffer_size == 64U) {
            transform(context, context->buffer);
            context->buffer_size = 0;
        }
        context->buffer[context->buffer_size++] = 0;
    }
    for (uint32_t i = 0; i < 8U; i++) context->buffer[56U + i] = (uint8_t)(bit_count >> (56U - i * 8U));
    transform(context, context->buffer);
    for (size_t i = 0; i < 8; i++) write_be32(digest + i * 4U, context->state[i]);
}

static char hex_digit(uint8_t value) {
    return value < 10U ? (char)('0' + value) : (char)('a' + value - 10U);
}

xboot_status_t xboot_sha256_hex(const uint8_t *data, size_t size,
                                char output[XBOOT_SHA256_HEX_SIZE]) {
    uint8_t digest[XBOOT_SHA256_DIGEST_SIZE];
    xboot_sha256_context context;
    if (data == NULL || output == NULL) return XBOOT_INVALID_ARGUMENT;
    xboot_sha256_init(&context);
    xboot_sha256_update(&context, data, size);
    xboot_sha256_final(&context, digest);
    for (size_t i = 0; i < XBOOT_SHA256_DIGEST_SIZE; i++) {
        output[i * 2U] = hex_digit((uint8_t)(digest[i] >> 4));
        output[i * 2U + 1U] = hex_digit((uint8_t)(digest[i] & 0x0fU));
    }
    output[XBOOT_SHA256_HEX_SIZE - 1U] = '\0';
    return XBOOT_OK;
}

xboot_status_t xboot_sha256_matches_hex(const uint8_t *data, size_t size,
                                        const char expected[XBOOT_SHA256_HEX_SIZE]) {
    char actual[XBOOT_SHA256_HEX_SIZE];
    uint8_t difference = 0;
    if (expected == NULL) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < XBOOT_SHA256_HEX_SIZE - 1U; i++) {
        char value = expected[i];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return XBOOT_CORRUPT;
    }
    if (expected[XBOOT_SHA256_HEX_SIZE - 1U] != '\0') return XBOOT_CORRUPT;
    if (xboot_sha256_hex(data, size, actual) != XBOOT_OK) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < XBOOT_SHA256_HEX_SIZE - 1U; i++) difference |= (uint8_t)(actual[i] ^ expected[i]);
    return difference == 0 ? XBOOT_OK : XBOOT_SECURITY_ERROR;
}
