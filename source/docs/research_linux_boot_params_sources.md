# Linux boot_params research notes

## Sources reviewed

Linux/x86 Boot Protocol: https://docs.kernel.org/arch/x86/boot.html
Linux zero page documentation: https://docs.kernel.org/arch/x86/zero-page.html

## Relevant offsets

The current protocol documentation defines `setup_header` fields including `type_of_loader` at 0x210, `loadflags` at 0x211, `ramdisk_image` at 0x218, `ramdisk_size` at 0x21c, `cmd_line_ptr` at 0x228, `initrd_addr_max` at 0x22c, `kernel_alignment` at 0x230, `relocatable_kernel` at 0x234, `xloadflags` at 0x236, `cmdline_size` at 0x238, `payload_offset` at 0x248, `payload_length` at 0x24c, `pref_address` at 0x258, `init_size` at 0x260, and `handover_offset` at 0x264.

The zero-page documentation defines `acpi_rsdp_addr` at offset 0x70, extended ramdisk image/size at 0xC0/0xC4, extended command-line pointer at 0xC8, secure_boot at 0x1EC, and `e820_entries`/`e820_table` at 0x1E8 and 0x2D0 respectively. The bootloader must fill the zero page according to the supported protocol and must not claim final Linux handoff until the ABI and entry mechanism are tested with a real kernel.
