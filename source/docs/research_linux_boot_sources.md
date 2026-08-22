# Linux/x86 boot protocol research notes

## Source reviewed

The Linux kernel documentation at https://docs.kernel.org/arch/x86/boot.html defines the x86 boot protocol and the real-mode setup header. The `HdrS` signature is at file offset `0x202`; the protocol version is at `0x206`; `setup_sects` is at `0x1f1` and defaults to four when zero. Modern protocol fields include `initrd_addr_max` at `0x22c`, `kernel_alignment` at `0x230`, `relocatable_kernel` at `0x234`, `xloadflags` at `0x236`, `cmdline_size` at `0x238`, `payload_offset` at `0x248`, `payload_length` at `0x24c`, `pref_address` at `0x258`, `init_size` at `0x260`, and `handover_offset` at `0x264`.

## Implementation implications

XBoot's first Linux milestone will be a read-only bzImage inspector. It will require the `HdrS` signature, reject unsupported protocol versions, normalize `setup_sects`, validate the setup area and payload ranges against the supplied image size, validate alignment and relocatability fields, and calculate a checked payload interval. It will not claim kernel placement, initramfs placement, boot-parameter construction, or final handoff until separate code and tests exist for those operations.
