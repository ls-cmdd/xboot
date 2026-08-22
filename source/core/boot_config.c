#include <xboot/boot_config.h>

static int equal(const uint8_t *a, size_t length, const char *b) {
    size_t i = 0;
    while (b[i] != '\0') {
        if (i >= length || a[i] != (uint8_t)b[i]) return 0;
        i++;
    }
    return i == length;
}

static void clear_string(char *value) {
    value[0] = '\0';
}

static xboot_status_t copy_value(const uint8_t *value, size_t length,
                                 char destination[XBOOT_CONFIG_STRING_MAX]) {
    while (length > 0 && (value[0] == ' ' || value[0] == '\t')) {
        value++;
        length--;
    }
    while (length > 0 && (value[length - 1] == ' ' || value[length - 1] == '\t')) length--;
    if (length == 0 || length >= XBOOT_CONFIG_STRING_MAX) return XBOOT_CORRUPT;
    for (size_t i = 0; i < length; i++) {
        if (value[i] == '\r' || value[i] == '\n' || value[i] == '\0') return XBOOT_CORRUPT;
        destination[i] = (char)value[i];
    }
    destination[length] = '\0';
    return XBOOT_OK;
}

static xboot_status_t parse_policy(const uint8_t *value, size_t length, uint8_t *required) {
    while (length > 0 && (value[0] == ' ' || value[0] == '\t')) {
        value++;
        length--;
    }
    while (length > 0 && (value[length - 1] == ' ' || value[length - 1] == '\t')) length--;
    if (equal(value, length, "required")) {
        *required = 1;
        return XBOOT_OK;
    }
    if (equal(value, length, "optional")) {
        *required = 0;
        return XBOOT_OK;
    }
    return XBOOT_CORRUPT;
}

xboot_status_t xboot_boot_config_parse(const uint8_t *text, size_t text_size,
                                       xboot_boot_entry *out_entry) {
    size_t offset = 0;
    uint8_t seen_title = 0;
    uint8_t seen_linux = 0;
    uint8_t seen_initrd = 0;
    uint8_t seen_options = 0;
    uint8_t seen_kernel_sha256 = 0;
    uint8_t seen_initrd_sha256 = 0;
    uint8_t seen_timeout = 0;
    uint8_t seen_measure = 0;
    uint8_t seen_secure_boot = 0;
    if (text == NULL || out_entry == NULL || text_size == 0) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < sizeof(*out_entry); i++) ((uint8_t *)out_entry)[i] = 0;
    clear_string(out_entry->title);
    clear_string(out_entry->linux_path);
    clear_string(out_entry->initrd_path);
    clear_string(out_entry->options);
    while (offset < text_size) {
        size_t line_start = offset;
        size_t line_end;
        size_t key_end;
        while (offset < text_size && text[offset] != '\n' && text[offset] != '\r') offset++;
        line_end = offset;
        while (offset < text_size && (text[offset] == '\n' || text[offset] == '\r')) offset++;
        while (line_start < line_end && (text[line_start] == ' ' || text[line_start] == '\t')) line_start++;
        if (line_start == line_end || text[line_start] == '#') continue;
        key_end = line_start;
        while (key_end < line_end && text[key_end] != '=') key_end++;
        if (key_end == line_start || key_end == line_end) return XBOOT_CORRUPT;
        size_t key_length = key_end - line_start;
        const uint8_t *value = text + key_end + 1;
        size_t value_length = line_end - key_end - 1;
        if (equal(text + line_start, key_length, "title")) {
            if (seen_title++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->title) != XBOOT_OK) return XBOOT_CORRUPT;
        } else if (equal(text + line_start, key_length, "linux")) {
            if (seen_linux++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->linux_path) != XBOOT_OK) return XBOOT_CORRUPT;
            out_entry->has_linux = 1;
        } else if (equal(text + line_start, key_length, "initrd")) {
            if (seen_initrd++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->initrd_path) != XBOOT_OK) return XBOOT_CORRUPT;
        } else if (equal(text + line_start, key_length, "options")) {
            if (seen_options++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->options) != XBOOT_OK) return XBOOT_CORRUPT;
        } else if (equal(text + line_start, key_length, "kernel_sha256")) {
            if (seen_kernel_sha256++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->kernel_sha256) != XBOOT_OK) return XBOOT_CORRUPT;
            out_entry->has_kernel_sha256 = 1;
        } else if (equal(text + line_start, key_length, "initrd_sha256")) {
            if (seen_initrd_sha256++) return XBOOT_CORRUPT;
            if (copy_value(value, value_length, out_entry->initrd_sha256) != XBOOT_OK) return XBOOT_CORRUPT;
            out_entry->has_initrd_sha256 = 1;
        } else if (equal(text + line_start, key_length, "measure")) {
            if (seen_measure++) return XBOOT_CORRUPT;
            if (parse_policy(value, value_length, &out_entry->measure_required) != XBOOT_OK) return XBOOT_CORRUPT;
        } else if (equal(text + line_start, key_length, "secure_boot")) {
            if (seen_secure_boot++) return XBOOT_CORRUPT;
            if (parse_policy(value, value_length, &out_entry->secure_boot_required) != XBOOT_OK) return XBOOT_CORRUPT;
        } else if (equal(text + line_start, key_length, "timeout")) {
            uint32_t timeout = 0;
            if (seen_timeout++) return XBOOT_CORRUPT;
            if (value_length == 0) return XBOOT_CORRUPT;
            for (size_t i = 0; i < value_length; i++) {
                if (value[i] < '0' || value[i] > '9' || timeout > (UINT32_MAX - (value[i] - '0')) / 10U) {
                    return XBOOT_CORRUPT;
                }
                timeout = timeout * 10U + (value[i] - '0');
            }
            out_entry->timeout_seconds = timeout;
        } else {
            return XBOOT_UNSUPPORTED;
        }
    }
    return out_entry->has_linux ? XBOOT_OK : XBOOT_NOT_FOUND;
}
