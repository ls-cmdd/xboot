# Testing

يستخدم XBoot طبقة host وطبقة QEMU/OVMF. تختبر طبقة المضيف parsers وامتلاك الذاكرة وواجهات UEFI المزيفة ومسار PE/COFF starter بصرامة `-Wall -Wextra -Werror -Wpedantic`. وتتحقق طبقة QEMU من تحميل `BOOTX64.EFI`، قراءة artifacts حقيقية من ESP، بناء boot parameters، ومسارين مستقلين لتشغيل Linux.

## البوابات

| الأمر | الدليل المطلوب | الوضع |
|---|---|---|
| `make clean host-test` | parsers، memory، UEFI adapters، Linux loader، real-image inspector، starter success/failure | PASS |
| `make test` | EFI-stub path، OVMF/q35، Ubuntu kernel/initrd، serial `Linux version` | PASS |
| `make test-linux-userspace` | EFI-stub path مع initramfs مصغر وmarkers `/init` | PASS |
| `make test-direct-linux` | direct build، host tests، direct protected-code/boot_params، final `ExitBootServices`، GDT/identity paging، Linux kernel وuserspace، ثم Ubuntu initrd الحقيقي | PASS |
| `make test-tcg2` | OVMF/q35 مع `swtpm` TPM2، TCG2 kernel/initrd measurements، PCR4 غير صفري في Linux sysfs، و`measure=required` no-TPM rejection | PASS |
| `make test-real-fat32` | يبني ESP حقيقية بـ `mkfs.fat`/`mkesp.sh`، ثم يركّبها parser FAT32 ويقرأ `BOOTX64.EFI` وshort alias لـ `xboot.conf` | PASS |
| `make fuzz-smoke` | ASan/UBSan على 100000 مدخل deterministic لـ config parser وLinux bzImage inspector | PASS |
| `make test-secureboot` | OVMF Secure Boot/SMM مع signed XBoot وsigned kernel، direct authentication delegation، وunsigned-kernel rejection | PASS باستخدام OVMF SnakeOil test keys |
| `tools/quality_gate.sh` | clean EFI-stub وuserspace وTCG2 وSecure Boot وdirect وhash-negative وreal-FAT32 وfuzz-smoke gates، forbidden-marker scan، unresolved-symbol check، وإعادة default non-direct image | PASS |

يُلتقط debug console في `build/qemu.log` أو `build/qemu.direct*.log`، بينما تُلتقط مخرجات Linux في `build/linux*.serial.log`. وجود `Linux version` يثبت دخول kernel فعلياً، و`Run /init as init process` وmarkers initramfs تثبت userspace؛ لا تُستخدم رسائل XBoot وحدها لإثبات الإقلاع.

## معنى direct gate

يبني `make test-direct-linux` نسخة `DIRECT=1` من XBoot، ويشغل initramfs مصغراً في QEMU `q35` مع 512MiB. بعد ذلك يعيد إنشاء ESP مع Ubuntu initrd الحقيقي ويثبت بدء Linux ووصوله إلى `/init`. في direct path لا يُستدعى UEFI بعد نجاح `ExitBootServices`; أي رجوع من assembly يُعامل كفشل.

## حدود الدليل

الدليل الحالي خاص بـ x86_64 وUEFI وOVMF/QEMU، ويستخدم Ubuntu kernel 6.8.0-138-generic. مسارا EFI-stub وdirect مثبتان على هذا kernel وQEMU `q35` فقط. ويثبت `make test-tcg2` TCG2 مع OVMF وTPM2 emulator من `swtpm`، ثم يقرأ initramfs PCR4 غير صفرياً من `/sys/class/tpm/tpm0/pcr-sha256/4`. لا يثبت ذلك توافق كل kernels أو firmware أو الأجهزة أو TPMs العتادية.

لا يطبق XBoot verifier مستقلاً لـ PE Authenticode/PKCS#7/X.509 أو إدارة PK/KEK/db/dbx؛ التنفيذ المثبت هو delegation إلى UEFI firmware عبر `LoadImage`/`UnloadImage` عند Secure Boot. أما `measure=required` فأصبح منفذاً ومختبراً مع OVMF+swtpm وnegative no-TPM. FAT32 core مثبت read-only لأسماء 8.3 على ESP حقيقية، لكن runtime الرئيسي ما زال UEFI SFS وLFN/كتابة FAT غير منفذة. تبقى NVMe/AHCI/USB controller drivers، BIOS، ARM64، RISC-V، physical-hardware matrix، production trust chain، وexternal certification غير مثبتة.
