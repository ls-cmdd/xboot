#ifndef XBOOT_X86_CPU_H
#define XBOOT_X86_CPU_H

#include <stdint.h>
#include <xboot/status.h>

typedef struct {
    char vendor[13];
    uint32_t max_basic_leaf;
    uint32_t max_extended_leaf;
    uint8_t physical_address_bits;
    uint8_t virtual_address_bits;
    uint8_t family;
    uint8_t model;
    uint8_t stepping;
    uint8_t cache_levels;
    uint16_t logical_processors;
    uint16_t cores_per_package;
    uint16_t threads_per_core;
    uint8_t has_apic;
    uint8_t has_x2apic;
    uint8_t has_tsc;
    uint8_t has_invariant_tsc;
    uint8_t has_nx;
    uint8_t has_smep;
    uint8_t has_smap;
    uint8_t has_vmx;
    uint8_t has_svm;
    uint8_t has_hypervisor;
    uint8_t topology_valid;
} xboot_x86_cpu_info;

xboot_status_t xboot_x86_cpu_discover(xboot_x86_cpu_info *out_info);

#endif
