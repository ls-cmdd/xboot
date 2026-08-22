# XBoot direct Linux boot design

## Decision

سيُنفذ direct backend وفق قسم **64-bit Boot Protocol** في وثائق Linux الرسمية، وليس عبر قفزة C عادية إلى protected code. وسيبقى PE/COFF EFI-stub backend منفصلاً ومدعوماً أثناء التطوير حتى لا يتحول المسار التجريبي إلى false success.

## Required Linux state

عند مدخل Linux 64-bit يجب أن يكون المعالج في long mode مع paging مفعّل، وأن تكون منطقة `init_size` ابتداءً من عنوان kernel، وzero page، وcommand line ذات identity mapping. يجب تحميل GDT فيها flat `__BOOT_CS=0x10` و`__BOOT_DS=0x18`، وضبط CS إلى code selector وDS/ES/SS إلى data selector، وتعطيل interrupts، ووضع عنوان `boot_params` في `RSI`. نقطة الدخول هي بداية loaded 64-bit kernel زائد `0x200` وفق protocol، بينما `handover_offset` يخص EFI handover المختلف والم deprecated.

## Image and memory layout

يجب إنشاء zero page بحجم 4096 ومحوها، ونسخ setup header حتى نهاية `0x0202 + setup_sects_byte_at_0x0201`، وتعبئة `cmd_line_ptr` و`ramdisk_image` و`ramdisk_size` وE820/EFI data وفق protocol. يجب تحميل protected-mode image بدءاً من `(setup_sects + 1) * 512`، مع التحقق من `syssize` و`init_size` و`kernel_alignment` و`pref_address` و`relocatable_kernel`، ثم حفظ reservations للkernel وinitrd وzero page وcommand line وXBoot code/data.

الإصدار الأول من direct backend سيستخدم placement منخفضاً أقل من 4GiB لتقليل مخاطر حقول zero-page القديمة، وسيُرفض صراحةً إذا تعذر تحقيق alignment أو identity mapping أو حدود initrd. لن تُستخدم `AllocateAnyPages` وحدها كدليل على صلاحية placement؛ يجب تسجيل العنوان الفعلي والتحقق من كل overflow وoverlap.

## Final firmware transaction

قبل `ExitBootServices` يتوقف XBoot عن كل allocation وprotocol operation غير ضرورية. يُجمع memory map نهائي في buffer محفوظ مسبقاً، ويُستخدم map key الموافق له. عند `EFI_INVALID_PARAMETER` يعاد جمع الخريطة ويعاد الاستدعاء دون أي allocation لاحق. بعد نجاح الخروج لا يجوز استدعاء UEFI، ويجب أن يكون كل ما يحتاجه assembly أو Linux موجوداً في ذاكرة محجوزة.

## Assembly boundary

ينفذ assembly wrapper فحصاً أخيراً للحالة، يبني identity page tables عند الحاجة، يثبت GDT، يعطل interrupts، يضبط selectors، يضع `RSI=boot_params`، يمسح السجلات المطلوبة، ثم يقفز إلى `kernel_address + 0x200`. لا يعود هذا المسار؛ أي return غير متوقع يذهب إلى halt loop مع marker debug مستقل. ستُختبر القطعة أولاً خارج firmware عبر fixture يراقب layout، ثم داخل QEMU مع serial marker من Linux.

## Acceptance gates

| Gate | Requirement |
|---|---|
| Host image gate | reject bad setup/syssize/init_size/alignment/overlap and accept real bzImage |
| Memory gate | every direct placement is below declared limits and disjoint from firmware reservations |
| Exit gate | final map is acquired after allocations freeze and `ExitBootServices` retry is deterministic |
| CPU gate | assembly entry uses documented long-mode state and `RSI` boot_params |
| QEMU gate | direct backend prints Linux version and reaches controlled `/init` |
| Regression gate | EFI-stub path and all previous host tests remain green |

## References

1. [The Linux/x86 Boot Protocol](https://docs.kernel.org/arch/x86/boot.html)
2. [The Linux EFI Boot Stub](https://docs.kernel.org/admin-guide/efi-stub.html)
3. [UEFI Specification 2.10](https://uefi.org/specifications)
