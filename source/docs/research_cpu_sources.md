# CPU discovery research notes

## Sources reviewed

1. Intel maintains the current Intel 64 and IA-32 Software Developer Manuals page at https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html. The manuals cover architecture, system programming, memory management, protection, interrupt handling, multiprocessor support, VMX, and model-specific registers. XBoot should use the manuals as normative references rather than compiler-specific assumptions.

2. Linux x86 topology documentation is at https://docs.kernel.org/arch/x86/topology.html. It distinguishes packages, cores, and threads and explains that architecture-specific topology details supplement generic topology definitions. XBoot's hardware graph should therefore preserve package/core/thread relationships without treating a physical socket name as a universal software identity.

## Implementation implications

The current CPUID path is only an initial discovery witness. The next CPU milestone must add explicit availability checks for extended leaves, physical and virtual address widths, APIC/x2APIC capability bits, TSC and invariant-TSC evidence where available, NX/SMEP/SMAP, virtualization vendor bits, cache/topology leaves, and a status for each unavailable or unsupported datum. No capability may be used merely because the executing CPU is expected to provide it.
