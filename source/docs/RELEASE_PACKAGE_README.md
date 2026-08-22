# XBoot Release Candidate

هذه الحزمة هي **release candidate محلي** لـ XBoot على x86_64 UEFI. تحتوي على EFI application وESP image مبنيين من commit مسجل، مع نتائج الاختبارات ووثائق الحدود. لا تمثل شهادة خارجية ولا تضمن توافقاً عاماً مع كل firmware أو الأجهزة.

## التثبيت السريع

للتجربة على آلة UEFI اختبارية، استخدم `build/xboot-esp.img` مع QEMU أو انسخ محتويات `EFI/` إلى EFI System Partition بعد أخذ نسخة احتياطية. يجب أن يكون المسار الافتراضي:

```text
EFI/BOOT/BOOTX64.EFI
```

ملفات Linux الافتراضية داخل الصورة هي `EFI/XBOOT/vmlinuz.efi` و`EFI/XBOOT/initrd.img`، ويحدد `EFI/XBOOT/xboot.conf` المسار والسياسات. لا تستخدم الصورة على جهاز إنتاجي قبل مراجعة المفاتيح وسياسة الإقلاع والنسخ الاحتياطي.

## التحقق من الحزمة

تحقق من `MANIFEST.sha256` قبل الاستخدام:

```bash
sha256sum -c MANIFEST.sha256
```

ويجب أن تعرض الملفات الأساسية `OK`. يمكن إعادة البناء والاختبار من شجرة المصدر باستخدام:

```bash
make clean
./tools/quality_gate.sh
```

## الميزات المثبتة

تثبت الحزمة مساري Linux مستقلين: PE/COFF EFI-stub وLinux direct 64-bit boot protocol. كما تتضمن SHA-256 artifact allowlist، TCG2 measurement اختياري و`measure=required` في OVMF+swtpm، Secure Boot enforcement مفوّضاً إلى firmware عبر `LoadImage`/`UnloadImage`، constrained allocation تحت 4GiB في direct path، parser FAT32 read-only محدوداً بأسماء 8.3، واختبار parser تحت ASan/UBSan.

## حدود الاستخدام

Secure Boot هنا **firmware-delegated**؛ لا يحتوي XBoot verifier مستقلاً لـ Authenticode أو إدارة PK/KEK/db/dbx، ومفتاح Secure Boot في witness هو OVMF SnakeOil الاختباري. لا تتضمن الحزمة drivers مستقلة لـ NVMe أو AHCI أو USB، ولا تدعي دعم BIOS أو ARM64 أو RISC-V. FAT32 runtime الرئيسي يعتمد UEFI Simple File System، وLFN وكتابة FAT غير منفذين.

نتائج TPM مثبتة مع OVMF/QEMU و`swtpm` فقط، وليست دليلاً على TPM عتادي. كما أن اختبار direct مثبت على مصفوفة Ubuntu kernel وOVMF/QEMU المسجلة، وليس شهادة توافق لكل kernels أو firmware. الاعتماد الخارجي يتطلب اختبار عتاد ومختبراً ومعياراً مستقلاً.

## الملفات

| المسار | الوصف |
|---|---|
| `EFI/BOOT/BOOTX64.EFI` | XBoot UEFI application الافتراضي |
| `EFI/XBOOT/vmlinuz.efi` | Linux artifact المسجل |
| `EFI/XBOOT/initrd.img` | initrd المسجل |
| `EFI/XBOOT/xboot.conf` | configuration hash-locked الافتراضي |
| `xboot-esp.img` | ESP image جاهزة للتجربة |
| `docs/` | سجل الأدلة والتدقيق والاختبارات |
| `source/` | شجرة المصدر والأدوات لإعادة البناء |
| `MANIFEST.sha256` | hashes لكل الملفات ضمن الحزمة |
