# Secure Boot research notes

توضح مواصفة UEFI 2.10 أن `GetVariable()` يقرأ متغيرات UEFI بالاسم وGUID، وأن `EFI_BUFFER_TOO_SMALL` يعيد الحجم المطلوب. كما تشرح المواصفة أن Secure Boot يعتمد على متغيرات السياسة وقواعد image verification، بينما يوضح دليل TianoCore أن `PK` و`KEK` و`db` و`dbx` تؤدي أدواراً مختلفة في سلسلة الثقة.

المكوّنات ذات الصلة هي `SecureBoot` و`SetupMode` تحت EFI Global Variable GUID، وقواعد التحقق من UEFI images عبر `db`/`dbx`. صورة PE/COFF تحمل certificate table داخل PE security directory، وتحدد UEFI صيغة WIN_CERTIFICATE وPKCS#7/Authenticode ومسار التحقق. لذلك لا يكفي فحص `SecureBoot=1` أو SHA-256 allowlist لادعاء Secure Boot.

الخطة الآمنة لـ XBoot: عند Secure Boot enabled، يجب ألا يسمح direct raw Linux path بتجاوز مصادقة firmware. المسار الأول المقترح هو استدعاء UEFI `LoadImage()` على نفس kernel path ثم `UnloadImage()` لاستخدام firmware image-authentication path قبل direct handoff، مع اختبار positive/negative على OVMF configured keys. هذا يثبت delegation إلى firmware ولا يساوي implementation مستقل لـ Authenticode parser. تنفيذ PE/COFF verifier مستقل يحتاج PE parsing وAuthenticode digest rules وPKCS#7/X.509/RSA verification وقواعد db/dbx، ولا ينبغي شحنه كـ stub.

References:

1. https://uefi.org/specs/UEFI/2.10/08_Services_Runtime_Services.html
2. https://uefi.org/specs/UEFI/2.10/32_Secure_Boot_and_Driver_Signing.html
3. https://tianocore-docs.github.io/Understanding_UEFI_Secure_Boot_Chain/draft/secure_boot_chain_in_uefi/uefi_secure_boot.html
