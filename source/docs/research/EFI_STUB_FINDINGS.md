# Linux EFI Stub findings

المصدر الرسمي: [The EFI Boot Stub](https://docs.kernel.org/admin-guide/efi-stub.html)، تمت قراءته في 2026-08-22.

توضح الوثيقة أن bzImage يمكن أن يتنكر كصورة PE/COFF وأن EFI firmware يستطيع تحميله كـ EFI executable، وأن هذا المسار لا يحتاج إلى GRUB أو محمل EFI تقليدي. يجب وضع الصورة على ESP وامتدادها `.efi` حتى يقبلها محمل EFI.

يدعم EFI stub خيار `initrd=`، ويجب أن يكون المسار مطلقاً من بداية ESP وبصيغة EFI مع backslash. كما تذكر الوثيقة إمكانية توفير initrd عبر بروتوكول UEFI خاص بـ Linux؛ تفاصيله موثقة في قسم PE/COFF entry point من Linux x86 boot protocol.

النتيجة التصميمية: مسار PE/COFF EFI stub هو المسار الأقل خطراً للانتقال من UEFI x86_64، لكنه يتطلب احترام PE/COFF section alignment وmemory footprint، وعدم اعتبار نسخ الملف الخام إلى صفحات مكافئاً لتحميل PE image. مسار boot protocol المباشر يبقى منفصلاً ويحتاج انتقال CPU مضبوطاً.

## PE/COFF entry point

المصدر الرسمي: [Linux/x86 Boot Protocol — PE/COFF entry point](https://docs.kernel.org/arch/x86/boot.html#pe-coff-entry-point). يذكر أن EFI stub يجعل bzImage صورة PE/COFF قابلة للتنفيذ كبرنامج EFI عادي، وأن استخدام Linux-specific `EFI_LOAD_FILE2` يتطلب handle يحمل البروتوكول وdevice path يعلن `LINUX_EFI_INITRD_MEDIA_GUID`. عندها يستدعي kernel `LoadFile2::LoadFile()` ويختار مكان initrd، فتتجنب XBoot معرفة قيود placement الداخلية الخاصة بـ boot_params.

الوثيقة نفسها تذكر أن EFI handover القديم deprecated لصالح PE/COFF entry point. لذلك سيكون مسار XBoot الأول للإقلاع الفعلي هو تحميل صورة kernel بصيغة PE/COFF عبر UEFI، مع احترام section alignment وimage footprint، وتسجيل initrd عبر البروتوكول القياسي عند الحاجة. ويبقى direct boot protocol مساراً مستقلاً لا يجوز خلطه بتحميل PE الخام.

## Direct boot protocol findings

المصدر الرسمي: [The Linux/x86 Boot Protocol](https://docs.kernel.org/arch/x86/boot.html)، تمت قراءة الأقسام الخاصة بـ `syssize` وmemory layout وRunning the Kernel في 2026-08-22.

تعرّف الوثيقة `syssize` عند offset `0x1f4` على أنه حجم protected-mode code بوحدات 16-byte paragraphs للإصدارات 2.04+. وتوضح أن real-mode code يتكون من boot sector وsetup code، وأن command line الحديثة يجب أن تكون NUL-terminated وضمن حدود `cmdline_size`، مع تجنب وضعها أسفل real-mode code أو في high memory.

بالنسبة إلى bzImage، يبدأ protected-mode code عند `(setup_sects + 1) * 512` ويُحمّل افتراضياً عند `0x100000`. وتوضح الوثيقة أن direct entry التقليدي يبدأ من real-mode kernel entry عند segment offset `0x20`، مع ضبط DS/ES/SS وFS/GS، stack صالح، وتعطيل interrupts. لذلك لا يجوز تحويل تطبيق UEFI long-mode إلى `code32_start` بقفزة C عادية؛ يلزم assembly transition موثق يضبط descriptor tables وCPU mode والحالة المطلوبة.

النتيجة التصميمية: سيبقى مسار PE/COFF EFI stub هو backend المدعوم حالياً، بينما direct boot protocol يحتاج backend منفصلاً مع low-memory real-mode setup placement، reservations دقيقة، final memory-map transaction، وassembly transition واختبار QEMU مستقل.
