#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <xboot/sha256.h>

int main(void) {
    static const uint8_t abc[] = {'a', 'b', 'c'};
    static const uint8_t empty[] = {0};
    static const char abc_hash[] = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    static const char empty_hash[] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    char output[XBOOT_SHA256_HEX_SIZE];
    assert(xboot_sha256_hex(abc, sizeof(abc), output) == XBOOT_OK);
    assert(strcmp(output, abc_hash) == 0);
    assert(xboot_sha256_matches_hex(abc, sizeof(abc), abc_hash) == XBOOT_OK);
    assert(xboot_sha256_matches_hex(abc, sizeof(abc), empty_hash) == XBOOT_SECURITY_ERROR);
    assert(xboot_sha256_hex(empty, 0, output) == XBOOT_OK);
    assert(strcmp(output, empty_hash) == 0);
    assert(xboot_sha256_matches_hex(empty, 0, "invalid") == XBOOT_CORRUPT);
    puts("SHA-256 host tests: PASS");
    return 0;
}
