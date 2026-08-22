# Architecture

يمتلك XBoot حالياً مسارين Linux مستقلين بعد مرحلة اكتشاف UEFI:

```text
UEFI firmware
    -> EFI application entry (XBoot)
    -> EFI system-table validation, watchdog, x86_64 discovery
    -> ACPI/SMBIOS inventory and bounded parsing
    -> UEFI Block I/O observation
    -> normalized UEFI memory-map snapshot
    -> UEFI Simple File System open on loaded-image device
    -> page-backed kernel/initrd reads
    -> bzImage HdrS/protocol/syssize/payload validation
    -> protected-code footprint + boot_params/E820 construction
    +--> default EFI-stub backend
    |      -> EFI FilePath device path
    |      -> UEFI LoadImage/StartImage
    |      -> Linux EFI stub -> Linux kernel/userspace
    |
    +--> DIRECT=1 backend
           -> pre-sized final map buffer
           -> stack + 4GiB identity page tables
           -> ExitBootServices retry transaction
           -> flat GDT (__BOOT_CS=0x10, __BOOT_DS=0x18)
           -> assembly RSI=boot_params, jump kernel_address+0x200
           -> Linux kernel/userspace
```

يمتلك `platform/uefi/main.c` دخول firmware ومسار الاكتشاف والتكامل. ويحتوي `include/xboot/uefi.h` على ABI الضروري فقط. يقرأ `platform/uefi/fs.c` artifacts من ESP، وتنفذ `loaders/linux/load.c` فحص image وplacement والـ zero page. يحتفظ `platform/uefi/handoff.c` بخريطة UEFI الخام وسعتها كي يضيف direct backend مساحة احتياطية قبل تجميد allocations.

في direct backend، ينفذ `loaders/linux/direct.c` حجز page tables وstack وبناء identity map لأول 4GiB، ثم يستدعي final `ExitBootServices` helper. بعد النجاح لا يستدعي XBoot أي UEFI service. تنفذ `arch/x86_64/linux_entry.S` الانتقال داخل long mode إلى GDT مسطحة، وتضع `RSI` على `boot_params`، ثم تقفز إلى مدخل Linux 64-bit. أي رجوع غير متوقع من هذا الحد يعامل كفشل بروتوكول.

المسار الافتراضي EFI-stub يبقى مهماً كـ backend مستقل ومتوافق مع UEFI، بينما direct path يثبت ownership transfer يدوياً. لا يستخدم أي مسار GRUB أو U-Boot أو systemd-boot أو Limine أو rEFInd.

يثبت `make test` و`make test-linux-userspace` مسار EFI-stub، ويثبت `make test-direct-linux` direct kernel/userspace وUbuntu initrd الحقيقي. ما زالت Secure Boot cryptographic verification، TPM measurement، NVMe/AHCI/USB drivers، BIOS، ARM64، RISC-V، ومصفوفة physical hardware خارج الأدلة الحالية.
