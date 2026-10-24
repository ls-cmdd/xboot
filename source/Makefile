SHELL := /bin/bash
PROJECT := xboot
BUILD := build
CC := clang
HOST_CC := clang
LD := lld-link
OBJCOPY := llvm-objcopy
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS_TEMPLATE ?= /usr/share/OVMF/OVMF_VARS_4M.fd
LINUX_KERNEL ?= assets/vmlinuz
LINUX_INITRD ?= assets/initrd.img
XBOOT_CONFIG ?= config/xboot-hash.conf

CFLAGS := --target=x86_64-pc-windows-msvc \
          -ffreestanding -fshort-wchar -fno-stack-protector -fno-builtin \
          -fno-asynchronous-unwind-tables -mno-red-zone \
          -Wall -Wextra -Werror -Wpedantic -Iinclude $(if $(DIRECT),-DXBOOT_DIRECT_LINUX,)
LDFLAGS := /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:x64

.PHONY: all clean image run test test-linux-userspace test-direct-linux test-hash-negative test-tcg2 test-secureboot test-real-fat32 fuzz-smoke host-test check

all: $(BUILD)/xboot.efi

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.obj: platform/uefi/main.c include/xboot/uefi.h include/xboot/handoff.h include/xboot/memory.h include/xboot/status.h include/xboot/boot_config.h config/xboot.conf | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/handoff.obj: platform/uefi/handoff.c include/xboot/handoff.h include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/memory_backend.obj: platform/uefi/memory_backend.c include/xboot/memory.h include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/page_alloc.obj: mm/page_alloc.c include/xboot/memory.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/resource.obj: core/resource.c include/xboot/resource.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/logger.obj: core/logger.c include/xboot/logger.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/x86_cpu.obj: arch/x86_64/cpu.c include/xboot/x86_cpu.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/acpi.obj: hardware/acpi/parser.c include/xboot/acpi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/smbios.obj: hardware/smbios/parser.c include/xboot/smbios.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/exit.obj: platform/uefi/exit.c include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/pci.obj: hardware/pci/enumerate.c include/xboot/pci.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/storage.obj: storage/block.c include/xboot/storage.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/fat32.obj: filesystems/fat32.c include/xboot/fat32.h include/xboot/storage.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/linux_image.obj: loaders/linux/inspect.c include/xboot/linux_image.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/boot_config.obj: core/boot_config.c include/xboot/boot_config.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/boot_manager.obj: core/boot_manager.c include/xboot/boot_manager.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/initramfs.obj: loaders/linux/initramfs.c include/xboot/initramfs.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/recovery.obj: core/recovery.c include/xboot/recovery.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/security.obj: core/security.c include/xboot/security.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/sha256.obj: core/sha256.c include/xboot/sha256.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/architecture.obj: core/architecture.c include/xboot/architecture.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/uefi_block.obj: platform/uefi/block.c include/xboot/uefi_block.h include/xboot/storage.h include/xboot/uefi.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gpt.obj: storage/gpt.c include/xboot/gpt.h include/xboot/storage.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/uefi_fs.obj: platform/uefi/fs.c include/xboot/uefi_fs.h include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/uefi_loader.obj: platform/uefi/loader.c include/xboot/uefi_loader.h include/xboot/uefi_fs.h include/xboot/memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/linux_loader.obj: loaders/linux/load.c include/xboot/linux_loader.h include/xboot/linux_image.h include/xboot/memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/linux_direct.obj: loaders/linux/direct.c include/xboot/linux_loader.h include/xboot/handoff.h include/xboot/memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/linux_entry.obj: arch/x86_64/linux_entry.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/uefi_linux.obj: platform/uefi/linux.c include/xboot/uefi_linux.h include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/uefi_tcg2.obj: platform/uefi/tcg2.c include/xboot/uefi_tcg2.h include/xboot/uefi.h include/xboot/status.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/xboot.efi: $(BUILD)/main.obj $(BUILD)/handoff.obj $(BUILD)/sha256.obj $(BUILD)/uefi_tcg2.obj $(BUILD)/memory_backend.obj $(BUILD)/page_alloc.obj $(BUILD)/resource.obj $(BUILD)/logger.obj $(BUILD)/x86_cpu.obj $(BUILD)/acpi.obj $(BUILD)/smbios.obj $(BUILD)/exit.obj $(BUILD)/pci.obj $(BUILD)/storage.obj $(BUILD)/fat32.obj $(BUILD)/linux_image.obj $(BUILD)/boot_config.obj $(BUILD)/boot_manager.obj $(BUILD)/initramfs.obj $(BUILD)/recovery.obj $(BUILD)/security.obj $(BUILD)/architecture.obj $(BUILD)/uefi_block.obj $(BUILD)/gpt.obj $(BUILD)/uefi_fs.obj $(BUILD)/uefi_loader.obj $(BUILD)/linux_loader.obj $(BUILD)/linux_direct.obj $(BUILD)/linux_entry.obj $(BUILD)/uefi_linux.obj
	$(LD) $(LDFLAGS) /out:$@ $^

$(BUILD)/xboot-esp.img: $(BUILD)/xboot.efi tools/mkesp.sh $(LINUX_KERNEL) $(LINUX_INITRD) $(XBOOT_CONFIG)
	./tools/mkesp.sh $(BUILD)/xboot.efi $@ $(LINUX_KERNEL) $(LINUX_INITRD) $(XBOOT_CONFIG)

image: $(BUILD)/xboot-esp.img

$(BUILD)/OVMF_VARS.fd: | $(BUILD)
	cp $(OVMF_VARS_TEMPLATE) $@

run: image $(BUILD)/OVMF_VARS.fd
	timeout 10s qemu-system-x86_64 \
		-machine q35 -m 256M -no-reboot -display none -monitor none \
		-debugcon stdio -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.fd \
		-drive format=raw,file=$(BUILD)/xboot-esp.img

check: all
	file $(BUILD)/xboot.efi
	llvm-readobj --file-headers --sections $(BUILD)/xboot.efi

host-test: | $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		mm/page_alloc.c tests/test_page_alloc.c -o $(BUILD)/test_page_alloc
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/resource.c core/logger.c tests/test_core.c -o $(BUILD)/test_core
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		arch/x86_64/cpu.c tests/test_x86_cpu.c -o $(BUILD)/test_x86_cpu
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		hardware/acpi/parser.c tests/test_acpi.c -o $(BUILD)/test_acpi
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		hardware/smbios/parser.c tests/test_smbios.c -o $(BUILD)/test_smbios
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/exit.c tests/test_exit.c -o $(BUILD)/test_exit
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/handoff.c tests/test_handoff.c -o $(BUILD)/test_handoff

	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		hardware/pci/enumerate.c tests/test_pci.c -o $(BUILD)/test_pci
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		storage/block.c tests/test_storage.c -o $(BUILD)/test_storage
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		filesystems/fat32.c storage/block.c tests/test_fat32.c -o $(BUILD)/test_fat32
		$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
			filesystems/fat32.c storage/block.c tests/test_real_fat32.c -o $(BUILD)/test_real_fat32
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		loaders/linux/inspect.c tests/test_linux_image.c -o $(BUILD)/test_linux_image
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/boot_config.c tests/test_boot_config.c -o $(BUILD)/test_boot_config
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/boot_manager.c tests/test_boot_manager.c -o $(BUILD)/test_boot_manager
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		loaders/linux/initramfs.c tests/test_initramfs.c -o $(BUILD)/test_initramfs
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/recovery.c tests/test_recovery.c -o $(BUILD)/test_recovery
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/security.c tests/test_security.c -o $(BUILD)/test_security
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/sha256.c tests/test_sha256.c -o $(BUILD)/test_sha256
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/architecture.c tests/test_architecture.c -o $(BUILD)/test_architecture
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/block.c storage/block.c tests/test_uefi_block.c -o $(BUILD)/test_uefi_block
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		storage/gpt.c storage/block.c tests/test_gpt.c -o $(BUILD)/test_gpt
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/fs.c tests/test_uefi_fs.c -o $(BUILD)/test_uefi_fs
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/loader.c platform/uefi/fs.c platform/uefi/memory_backend.c mm/page_alloc.c tests/test_uefi_loader.c -o $(BUILD)/test_uefi_loader
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/linux.c tests/test_uefi_linux.c -o $(BUILD)/test_uefi_linux

	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		loaders/linux/load.c loaders/linux/inspect.c mm/page_alloc.c tests/test_linux_loader.c -o $(BUILD)/test_linux_loader
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		loaders/linux/inspect.c tests/test_real_linux_image.c -o $(BUILD)/test_real_linux_image
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		core/boot_config.c tests/test_real_boot_config.c -o $(BUILD)/test_real_boot_config
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude \
		platform/uefi/tcg2.c tests/test_tcg2.c -o $(BUILD)/test_tcg2
	$(BUILD)/test_page_alloc
	$(BUILD)/test_core
	$(BUILD)/test_x86_cpu
	$(BUILD)/test_acpi
	$(BUILD)/test_smbios
	$(BUILD)/test_exit
	$(BUILD)/test_handoff
	$(BUILD)/test_pci
	$(BUILD)/test_storage
	$(BUILD)/test_fat32
	$(BUILD)/test_linux_image
	$(BUILD)/test_boot_config
	$(BUILD)/test_boot_manager
	$(BUILD)/test_initramfs
	$(BUILD)/test_recovery
	$(BUILD)/test_security
	$(BUILD)/test_sha256
	$(BUILD)/test_architecture
	$(BUILD)/test_uefi_block
	$(BUILD)/test_gpt
	$(BUILD)/test_uefi_fs
	$(BUILD)/test_uefi_loader
	$(BUILD)/test_uefi_linux
	$(BUILD)/test_linux_loader
	$(BUILD)/test_real_linux_image
	$(BUILD)/test_real_boot_config
	$(BUILD)/test_tcg2

$(BUILD)/test-initrd.img: tools/mk-test-initrd.sh | $(BUILD)
	./tools/mk-test-initrd.sh $@

$(BUILD)/xboot-userspace-esp.img: $(BUILD)/xboot.efi $(BUILD)/test-initrd.img tools/mkesp.sh $(XBOOT_CONFIG)
	./tools/mkesp.sh $(BUILD)/xboot.efi $@ assets/vmlinuz $(BUILD)/test-initrd.img config/xboot.conf

test-direct-linux:
	$(MAKE) clean DIRECT=1 host-test
	$(MAKE) DIRECT=1 build/test-initrd.img
	rm -f $(BUILD)/xboot-esp.img
	$(MAKE) DIRECT=1 LINUX_INITRD=$(BUILD)/test-initrd.img XBOOT_CONFIG=config/xboot.conf image
	cp $(OVMF_VARS_TEMPLATE) $(BUILD)/OVMF_VARS.direct.fd
	rm -f $(BUILD)/qemu.direct.log $(BUILD)/linux.direct.serial.log
	(timeout 15s qemu-system-x86_64 \
		-machine q35 -m 512M -no-reboot -display none -monitor none \
		-serial file:$(BUILD)/linux.direct.serial.log \
		-debugcon file:$(BUILD)/qemu.direct.log -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.direct.fd \
		-drive format=raw,file=$(BUILD)/xboot-esp.img >/dev/null 2>&1 || true)
	grep -q "Starting Linux through direct 64-bit boot protocol" $(BUILD)/qemu.direct.log
	grep -q "TCG2 measurement unavailable" $(BUILD)/qemu.direct.log
	! grep -q "direct Linux entry returned" $(BUILD)/qemu.direct.log
	grep -a -q "Linux version" $(BUILD)/linux.direct.serial.log
	grep -a -q "XBOOT_LINUX_USERSPACE_OK" $(BUILD)/linux.direct.serial.log
	grep -a -q "XBOOT_INITRAMFS_ROOT_READY" $(BUILD)/linux.direct.serial.log
	rm -f $(BUILD)/xboot-esp.img
	$(MAKE) DIRECT=1 image
	cp $(OVMF_VARS_TEMPLATE) $(BUILD)/OVMF_VARS.direct.real.fd
	rm -f $(BUILD)/qemu.direct.real.log $(BUILD)/linux.direct.real.serial.log
	(timeout 60s qemu-system-x86_64 \
		-machine q35 -m 512M -no-reboot -display none -monitor none \
		-serial file:$(BUILD)/linux.direct.real.serial.log \
		-debugcon file:$(BUILD)/qemu.direct.real.log -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.direct.real.fd \
		-drive format=raw,file=$(BUILD)/xboot-esp.img >/dev/null 2>&1 || true)
	grep -a -q "Linux version" $(BUILD)/linux.direct.real.serial.log
	grep -q "Starting Linux through direct 64-bit boot protocol" $(BUILD)/qemu.direct.real.log
	grep -a -q "Run /init as init process" $(BUILD)/linux.direct.real.serial.log

test-hash-negative: $(BUILD)/xboot.efi $(LINUX_KERNEL) $(LINUX_INITRD) config/xboot-hash.conf
	cp $(LINUX_KERNEL) $(BUILD)/tampered-vmlinuz
	printf '\\001' | dd of=$(BUILD)/tampered-vmlinuz bs=1 seek=4096 conv=notrunc status=none
	rm -f $(BUILD)/xboot-esp.img
	$(MAKE) LINUX_KERNEL=$(BUILD)/tampered-vmlinuz XBOOT_CONFIG=config/xboot-hash.conf image
	cp $(OVMF_VARS_TEMPLATE) $(BUILD)/OVMF_VARS.hash-negative.fd
	rm -f $(BUILD)/qemu.hash-negative.log $(BUILD)/linux.hash-negative.serial.log
	(timeout 60s qemu-system-x86_64 \
		-machine q35 -m 512M -no-reboot -display none -monitor none \
		-serial file:$(BUILD)/linux.hash-negative.serial.log \
		-debugcon file:$(BUILD)/qemu.hash-negative.log -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.hash-negative.fd \
		-drive format=raw,file=$(BUILD)/xboot-esp.img >/dev/null 2>&1 || true)
	grep -q "Linux artifact validation failed: security error" $(BUILD)/qemu.hash-negative.log
	! grep -a -q "Linux version" $(BUILD)/linux.hash-negative.serial.log

fuzz-smoke: | $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
		core/boot_config.c loaders/linux/inspect.c tests/test_fuzz_smoke.c -o $(BUILD)/test_fuzz_smoke
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(BUILD)/test_fuzz_smoke

test-real-fat32:
	$(MAKE) clean image
	$(MAKE) host-test
	$(BUILD)/test_real_fat32 $(BUILD)/xboot-esp.img

test-secureboot: config/xboot-secureboot-required.conf
	$(MAKE) clean
	$(MAKE) DIRECT=1
	chmod +x tools/run-secureboot-test.sh
	./tools/run-secureboot-test.sh

test-tcg2: host-test $(BUILD)/test-initrd.img config/xboot-tcg2-required.conf
	$(MAKE) LINUX_INITRD=$(BUILD)/test-initrd.img XBOOT_CONFIG=config/xboot-tcg2-required.conf image
	chmod +x tools/run-tcg2-test.sh tools/run-tcg2-required-negative.sh
	./tools/run-tcg2-test.sh $(BUILD)/xboot-esp.img
	./tools/run-tcg2-required-negative.sh $(BUILD)/xboot-esp.img

test-linux-userspace: $(BUILD)/xboot-userspace-esp.img
	cp $(OVMF_VARS_TEMPLATE) $(BUILD)/OVMF_VARS.userspace.fd
	rm -f $(BUILD)/linux.userspace.serial.log
	(timeout 15s qemu-system-x86_64 \
		-machine q35 -m 256M -no-reboot -display none -monitor none \
		-serial file:$(BUILD)/linux.userspace.serial.log \
		-debugcon file:$(BUILD)/qemu-userspace.log -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.userspace.fd \
		-drive format=raw,file=$(BUILD)/xboot-userspace-esp.img >/dev/null 2>&1 || true)
	grep -a -q "Linux version" $(BUILD)/linux.userspace.serial.log
	grep -a -q "XBOOT_LINUX_USERSPACE_OK" $(BUILD)/linux.userspace.serial.log
	grep -a -q "XBOOT_INITRAMFS_ROOT_READY" $(BUILD)/linux.userspace.serial.log

test: host-test image $(BUILD)/OVMF_VARS.fd
	cp $(OVMF_VARS_TEMPLATE) $(BUILD)/OVMF_VARS.test.fd
	rm -f $(BUILD)/qemu.log $(BUILD)/linux.serial.log

	(timeout 60s qemu-system-x86_64 \
		-machine q35 -m 512M -no-reboot -display none -monitor none \
		-serial file:$(BUILD)/linux.serial.log \
		-debugcon file:$(BUILD)/qemu.log -global isa-debugcon.iobase=0x402 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.test.fd \
		-drive format=raw,file=$(BUILD)/xboot-esp.img >/dev/null 2>&1 || true)
	grep -q "XBoot phase 1 - UEFI x86_64 entry" $(BUILD)/qemu.log
	grep -q "EFI system table validated" $(BUILD)/qemu.log
	grep -q "handoff memory regions:" $(BUILD)/qemu.log
	grep -q "page allocator lifecycle verified" $(BUILD)/qemu.log
	grep -q "Linux artifact validation completed" $(BUILD)/qemu.log
	grep -q "TCG2 measurement unavailable" $(BUILD)/qemu.log
	grep -a -q "Linux version" $(BUILD)/linux.serial.log

clean:
	rm -rf $(BUILD)
