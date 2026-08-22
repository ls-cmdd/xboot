# نتائج قياس TCG2 في XBoot

## النطاق والمرجعية

تعرّف مواصفة **TCG EFI Protocol** واجهة UEFI قياسية للاستعلام عن وجود TPM، ومعرفة PCR banks النشطة، والحصول على سجل الإقلاع، وتمديد hashes إلى PCRs، وإضافة الأحداث إلى السجل [1]. ويستخدم XBoot في هذه المرحلة `EFI_TCG2_PROTOCOL` من خلال `LocateProtocol` ثم `GetCapability` و`HashLogExtendEvent`، من دون تنفيذ TPM command engine خاص به.

تمت مطابقة تعريفات ABI مع ترويسة EDK2 المرجعية: GUID هو `607f766c-7455-42be-930b-e4d76db2720f`، و`EFI_TCG2_EVENT_HEADER` و`EFI_TCG2_EVENT` موضوعان تحت `#pragma pack(1)`، بينما capability structure غير packed وفق تعريف EDK2 [2]. يطلب XBoot أن تكون `TPMPresentFlag` مفعلة وأن يكون SHA-256 موجوداً في كل من `HashAlgorithmBitmap` و`ActivePcrBanks`. ويُرسل الحدث إلى **PCR 4** بنوع `EV_IPL` وبـ`Flags=0`، أي إن الطلب يتيح للـ firmware إجراء extend وتسجيل event معاً.

## التنفيذ

ينشئ `platform/uefi/tcg2.c` event descriptor bounded من النص الوصفي، ويستخدم العنوان الفيزيائي وطول artifact الأصليين في استدعاء `HashLogExtendEvent`. يقيس runtime حالياً kernel وinitrd بعد تحميلهما والتحقق من allowlist، وقبل تجهيز Linux handoff. في الوضع `measure=optional`، إذا لم يوجد protocol أو لم يكن TPM/SHA-256 متاحاً، تُسجّل الحالة كـ unavailable ويستمر الإقلاع؛ وإذا أعاد firmware خطأ عتادياً، تُسجل failure ولا تُستبدل النتيجة برسالة نجاح مصطنعة. في الوضع `measure=required`، يعيد XBoot status الفشل ويحرر artifacts قبل أي Linux handoff، لذلك يمنع غياب TPM أو فشل القياس الإقلاع.

تختبر `tests/test_tcg2.c` GUID، ABI callback، PCR index، event type، header version/size، event total size، physical data address/length، حالات protocol غير الموجود، TPM غير الموجود، SHA-256 bank غير النشطة، ونجاح callback. ويُبنى adapter ضمن `xboot.efi` وليس dead code.

## Execution witness

ينفذ target:

```text
make test-tcg2
```

ويبني ESP بــ `config/xboot.conf` وinitramfs اختباري، ثم يشغل OVMF 2024.02 على QEMU `q35` بذاكرة 256MiB مع TPM2 emulator من `swtpm`. يثبت debug log علامتي:

```text
TCG2 measured Linux kernel into PCR4
TCG2 measured Linux initrd into PCR4
```

ويقرأ initramfs داخل Linux الملف:

```text
/sys/class/tpm/tpm0/pcr-sha256/4
```

ويشترط أن يكون PCR4 متاحاً وغير صفري. إحدى نتائج التنفيذ المسجلة في الاختبار كانت:

```text
PCR4: 089899220CCD44B1557C1E66D1DA50AE2B7C580DB1E1FF1F2F2C80CAD221CE03
```

كما يمر `tools/quality_gate.sh` بعد إدراج `make test-tcg2` إلى جانب EFI-stub وdirect وuserspace وhash-negative gates. هذا الدليل يثبت مسار TCG2 فعلياً في **OVMF/QEMU مع swtpm**، لا مجرد تسجيل نص في debug console؛ لكنه لا يثبت وجود TPM عتادي أو توافق كل firmware.

> وجود PCR4 غير صفري في Linux sysfs مع markers القياس قبل EFI-stub يثبت أن firmware/TPM path تعامل مع القياس فعلياً في هذا السيناريو. لا ينبغي تفسير القيمة كـ golden PCR ثابت، لأن PCR يبدأ أيضاً بقياسات firmware وقد يختلف بتغير OVMF أو حالة TPM.

## الحدود الأمنية

هذا التنفيذ هو **TCG2 measurement backend تجريبي**، وليس Secure Boot. لا يقرأ XBoot PK/KEK/db/dbx، ولا يتحقق من PE Authenticode certificate chain، ولا يثبت trust anchor أو anti-rollback policy. كما أن SHA-256 allowlist الموجودة في `xboot-hash.conf` تضمن سلامة artifacts المعروفة فقط، ولا تتحول إلى شهادة توقيع بمجرد استخدامها مع TCG2.

كذلك لا يوجد حالياً event-log export أو verifier مستقل خارج guest، ولا تكامل مع remote attestation أو policy key management. هذه العناصر تبقى أعمالاً لاحقة قبل أي claim عن measured-boot certification أو remote attestation.

## التصنيف

| Capability | الحالة | الدليل | الحد |
|---|---|---|---|
| TCG2 ABI adapter | Implemented | host tests وEFI link | ABI subset مطلوب للقياس فقط |
| Kernel/initrd measurement | Experimental PASS | `make test-tcg2` مع OVMF+swtpm وPCR4 من Linux sysfs | QEMU emulator فقط |
| Required measurement policy | Implemented/tested | `measure=required` في config مع negative no-TPM gate | policy مستقلة عن default optional |
| Secure Boot | Unsupported | لا PK/KEK/db/dbx ولا Authenticode | مستقل عن TCG2 |
| Hardware TPM matrix | Unverified | لا توجد منصة فعلية | يلزم اختبار عتادي موثق |

## References

1. [TCG EFI Protocol Specification — Trusted Computing Group](https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/)
2. [EDK2 `MdePkg/Include/Protocol/Tcg2Protocol.h`](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/Tcg2Protocol.h)
3. [QEMU TPM Device Specification](https://qemu-project.gitlab.io/qemu/specs/tpm.html)
4. [SUSE Virtualization Guide — Software TPM emulator](https://documentation.suse.com/sles/15-SP6/html/SLES-all/tpm.html)
