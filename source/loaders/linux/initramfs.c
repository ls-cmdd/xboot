#include <xboot/initramfs.h>

#define CPIO_HEADER_SIZE 110U
#define CPIO_MAX_NAME 4096U
#define CPIO_MAX_ENTRIES 65536U

static int equal(const uint8_t *a, const char *b) {
    for (size_t i = 0; b[i] != '\0'; i++) if (a[i] != (uint8_t)b[i]) return 0;
    return 1;
}

static xboot_status_t parse_hex(const uint8_t *p, size_t width, uint32_t *out) {
    uint32_t value = 0;
    for (size_t i = 0; i < width; i++) {
        uint8_t c = p[i];
        uint8_t digit;
        if (c >= '0' && c <= '9') digit = (uint8_t)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (uint8_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = (uint8_t)(c - 'A' + 10);
        else return XBOOT_CORRUPT;
        if (value > (UINT32_MAX - digit) / 16U) return XBOOT_CORRUPT;
        value = value * 16U + digit;
    }
    *out = value;
    return XBOOT_OK;
}

static size_t align4(size_t value) {
    return (value + 3U) & ~3U;
}

xboot_status_t xboot_initramfs_inspect(const uint8_t *archive, size_t archive_size,
                                       xboot_initramfs_entry_callback callback,
                                       void *context, uint32_t *out_entry_count) {
    size_t offset = 0;
    uint32_t count = 0;
    if (archive == NULL || archive_size == 0 || out_entry_count == NULL) return XBOOT_INVALID_ARGUMENT;
    while (offset < archive_size) {
        const uint8_t *header;
        uint32_t mode, uid, gid, filesize, namesize, checksum;
        size_t name_offset;
        size_t data_offset;
        xboot_initramfs_entry entry;
        int is_crc;
        while (offset < archive_size && archive[offset] == 0) offset++;
        if (offset == archive_size) break;
        if (archive_size - offset < CPIO_HEADER_SIZE) return XBOOT_CORRUPT;
        header = archive + offset;
        if (!equal(header, "070701") && !equal(header, "070702")) return XBOOT_PROTOCOL_ERROR;
        is_crc = header[5] == '2';
        if (parse_hex(header + 14, 8, &mode) != XBOOT_OK ||
            parse_hex(header + 22, 8, &uid) != XBOOT_OK ||
            parse_hex(header + 30, 8, &gid) != XBOOT_OK ||
            parse_hex(header + 54, 8, &filesize) != XBOOT_OK ||
            parse_hex(header + 94, 8, &namesize) != XBOOT_OK ||
            parse_hex(header + 102, 8, &checksum) != XBOOT_OK) return XBOOT_CORRUPT;
        if (namesize == 0 || namesize > CPIO_MAX_NAME || namesize > archive_size - offset - CPIO_HEADER_SIZE) return XBOOT_CORRUPT;
        name_offset = align4(offset + CPIO_HEADER_SIZE + namesize);
        if (name_offset > archive_size || archive[offset + CPIO_HEADER_SIZE + namesize - 1] != 0) return XBOOT_CORRUPT;
        if (filesize > archive_size - name_offset) return XBOOT_CORRUPT;
        data_offset = name_offset;
        entry.mode = mode;
        entry.uid = uid;
        entry.gid = gid;
        entry.filesize = filesize;
        entry.namesize = namesize;
        entry.checksum = checksum;
        entry.is_crc = (uint8_t)is_crc;
        entry.is_trailer = (uint8_t)(namesize == 11 && equal(archive + offset + CPIO_HEADER_SIZE, "TRAILER!!!"));
        entry.name = archive + offset + CPIO_HEADER_SIZE;
        entry.data = archive + data_offset;
        if (entry.is_trailer) {
            if (filesize != 0) return XBOOT_CORRUPT;
            *out_entry_count = count;
            return XBOOT_OK;
        }
        if (is_crc) {
            uint32_t sum = 0;
            for (uint32_t i = 0; i < filesize; i++) sum += entry.data[i];
            if (sum != checksum) return XBOOT_CORRUPT;
        } else if (checksum != 0) {
            return XBOOT_CORRUPT;
        }
        if (count >= CPIO_MAX_ENTRIES) return XBOOT_CORRUPT;
        if (callback != NULL) {
            xboot_status_t status = callback(context, &entry);
            if (status != XBOOT_OK) return status;
        }
        count++;
        offset = align4(data_offset + filesize);
        if (offset > archive_size) return XBOOT_CORRUPT;
    }
    *out_entry_count = count;
    return XBOOT_OK;
}
