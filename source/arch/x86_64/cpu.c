#include <stddef.h>
#include <xboot/x86_cpu.h>

static void cpuid(uint32_t leaf, uint32_t subleaf,
                  uint32_t *eax, uint32_t *ebx,
                  uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(subleaf));
}

static void clear_info(xboot_x86_cpu_info *info) {
    uint8_t *bytes = (uint8_t *)info;
    for (size_t i = 0; i < sizeof(*info); i++) {
        bytes[i] = 0;
    }
}

static void copy_vendor(char vendor[13], uint32_t ebx, uint32_t edx, uint32_t ecx) {
    vendor[0] = (char)(ebx & 0xff); vendor[1] = (char)((ebx >> 8) & 0xff);
    vendor[2] = (char)((ebx >> 16) & 0xff); vendor[3] = (char)((ebx >> 24) & 0xff);
    vendor[4] = (char)(edx & 0xff); vendor[5] = (char)((edx >> 8) & 0xff);
    vendor[6] = (char)((edx >> 16) & 0xff); vendor[7] = (char)((edx >> 24) & 0xff);
    vendor[8] = (char)(ecx & 0xff); vendor[9] = (char)((ecx >> 8) & 0xff);
    vendor[10] = (char)((ecx >> 16) & 0xff); vendor[11] = (char)((ecx >> 24) & 0xff);
    vendor[12] = '\0';
}

static void decode_version(xboot_x86_cpu_info *info, uint32_t eax) {
    uint32_t base_family = (eax >> 8) & 0xf;
    uint32_t base_model = (eax >> 4) & 0xf;
    uint32_t extended_family = (eax >> 20) & 0xff;
    uint32_t extended_model = (eax >> 16) & 0xf;
    info->stepping = (uint8_t)(eax & 0xf);
    info->family = (uint8_t)(base_family == 0xf ? base_family + extended_family : base_family);
    info->model = (uint8_t)((base_family == 0x6 || base_family == 0xf)
                                ? base_model | (extended_model << 4)
                                : base_model);
}

static void decode_topology(xboot_x86_cpu_info *info) {
    uint32_t leaf = 0;
    uint32_t eax, ebx, ecx, edx;
    uint32_t subleaf;
    uint32_t logical = 0;
    uint32_t smt_logical = 0;
    uint32_t core_logical = 0;
    uint8_t found = 0;

    if (info->max_basic_leaf >= 0x1f) {
        leaf = 0x1f;
    } else if (info->max_basic_leaf >= 0x0b) {
        leaf = 0x0b;
    } else {
        return;
    }
    for (subleaf = 0; subleaf < 8; subleaf++) {
        cpuid(leaf, subleaf, &eax, &ebx, &ecx, &edx);
        uint32_t level_type = (ecx >> 8) & 0xff;
        uint32_t level_logical = ebx & 0xffff;
        if (level_type == 0 || level_logical == 0) {
            break;
        }
        found = 1;
        logical = level_logical;
        if (level_type == 1) {
            smt_logical = level_logical;
        } else if (level_type == 2) {
            core_logical = level_logical;
        }
    }
    if (!found || logical == 0) {
        return;
    }
    info->logical_processors = logical > UINT16_MAX ? UINT16_MAX : (uint16_t)logical;
    if (smt_logical != 0 && core_logical >= smt_logical) {
        uint32_t cores = core_logical / smt_logical;
        info->threads_per_core = smt_logical > UINT16_MAX ? UINT16_MAX : (uint16_t)smt_logical;
        info->cores_per_package = cores > UINT16_MAX ? UINT16_MAX : (uint16_t)cores;
    } else {
        info->cores_per_package = info->logical_processors;
        info->threads_per_core = 1;
    }
    info->topology_valid = 1;
}

xboot_status_t xboot_x86_cpu_discover(xboot_x86_cpu_info *out_info) {
    uint32_t eax, ebx, ecx, edx;
    if (out_info == NULL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    clear_info(out_info);
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    out_info->max_basic_leaf = eax;
    copy_vendor(out_info->vendor, ebx, edx, ecx);
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    out_info->max_extended_leaf = eax;

    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    decode_version(out_info, eax);
    out_info->has_apic = (uint8_t)((edx >> 9) & 1);
    out_info->has_tsc = (uint8_t)((edx >> 4) & 1);
    out_info->has_x2apic = (uint8_t)((ecx >> 21) & 1);
    out_info->has_vmx = (uint8_t)((ecx >> 5) & 1);
    out_info->has_hypervisor = (uint8_t)((ecx >> 31) & 1);

    if (out_info->max_basic_leaf >= 7) {
        cpuid(7, 0, &eax, &ebx, &ecx, &edx);
        out_info->has_smep = (uint8_t)((ebx >> 7) & 1);
        out_info->has_smap = (uint8_t)((ebx >> 20) & 1);
    }
    if (out_info->max_extended_leaf >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        out_info->has_nx = (uint8_t)((edx >> 20) & 1);
        out_info->has_svm = (uint8_t)((ecx >> 2) & 1);
    }
    if (out_info->max_extended_leaf >= 0x80000007) {
        cpuid(0x80000007, 0, &eax, &ebx, &ecx, &edx);
        out_info->has_invariant_tsc = (uint8_t)((edx >> 8) & 1);
    }
    if (out_info->max_extended_leaf >= 0x80000008) {
        cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        out_info->physical_address_bits = (uint8_t)(eax & 0xff);
        out_info->virtual_address_bits = (uint8_t)((eax >> 8) & 0xff);
    }
    decode_topology(out_info);
    return XBOOT_OK;
}
