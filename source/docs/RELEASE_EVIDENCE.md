# XBoot release evidence record

## Scope

هذا السجل يصف **release-candidate محلياً** لمجال x86_64 UEFI تحت OVMF/QEMU. لا يمثل شهادة خارجية ولا تقرير توافق hardware عاماً، ولا يحول نتائج QEMU أو swtpm أو مفاتيح SnakeOil الاختبارية إلى اعتماد إنتاجي.

| Item | Value |
|---|---|
| Repository | `/home/ubuntu/xboot` |
| Firmware code | `/usr/share/OVMF/OVMF_CODE_4M.fd` و`OVMF_CODE_4M.snakeoil.fd`، OVMF 2024.02 |
| QEMU machine | `q35`؛ Secure Boot witness يستخدم SMM |
| Tested memory | 256MiB EFI-stub/userspace/TCG2 و512MiB direct |
| Kernel asset SHA-256 | `d74be574189057a036866309fd5546724a92390a4f6ba086e93ddab2f6e0e01d` |
| Initrd asset SHA-256 | `79c1188d6ca7fd06398b4ccd26615567047bfe47a21faaff84d79327b96aca06` |
| TPM test backend | TPM2 emulator `swtpm` مع OVMF TCG2 protocol |
| Secure Boot test trust | OVMF SnakeOil test PK/KEK/db variables؛ ليس OEM أو Microsoft production trust |

## Executed gates

| Command | Result | Witness |
|---|---|---|
| `make clean host-test` | PASS | memory/parsers/UEFI adapters/Linux loader/real-image/TCG2/Secure Boot delegation tests pass |
| `make test` | PASS | default hash-locked EFI-stub path starts Ubuntu Linux؛ serial contains `Linux version` |
| `make test-linux-userspace` | PASS | EFI-stub QEMU يصل controlled `/init` markers |
| `make test-tcg2` | PASS | OVMF+QEMU+swtpm يقيس kernel/initrd إلى PCR4؛ initramfs يقرأ PCR4 غير صفري؛ `measure=required` يمنع no-TPM boot |
| `make test-secureboot` | PASS | signed XBoot/kernel يجتاز Secure Boot delegated authentication؛ unsigned kernel يُرفض ولا يظهر `Linux version` |
| `make test-direct-linux` | PASS | direct 64-bit entry، constrained allocations تحت 4GiB، final ExitBootServices، controlled userspace، وUbuntu initrd start |
| `make test-hash-negative` | PASS | tampered kernel rejected fail-closed قبل Linux version |
| `make test-real-fat32` | PASS | parser FAT32 يركّب ESP 128MiB حقيقية ويقرأ BOOTX64.EFI وshort alias لـ xboot.conf |
| `make fuzz-smoke` | PASS | ASan/UBSan parser smoke على 100000 deterministic inputs |
| `tools/quality_gate.sh` | PASS | كل gates أعلاه، real-FAT32، fuzz-smoke، forbidden-marker scan، unresolved-symbol check، وإعادة default non-direct image |

## EFI-stub witness

مسار EFI-stub يبني UEFI Device Path من parent DeviceHandle ثم يستدعي `LoadImage` و`HandleProtocol` و`LoadOptions` و`StartImage` للـ Linux PE/COFF EFI stub. يثبت QEMU وصول Ubuntu 6.8.0-138-generic إلى Linux، وتثبت نسخة initramfs المضبوطة markers `/init`.

## Direct-path witness

يحتوي direct QEMU log على `Starting Linux through direct 64-bit boot protocol`. ويحتوي serial log على `Linux version 6.8.0-138-generic` وmarkers initramfs. وتشغّل نسخة Ubuntu initrd الحقيقية حتى `Run /init as init process`، مع بقاء root disk غير موجود ولذلك لا يُدّعى اكتمال userspace على قرص فعلي.

يستخدم direct build الآن backend UEFI مستقلاً يستدعي `AllocateMaxAddress` بحد `0xFFFFFFFF` قبل كل allocations، بدلاً من تخصيص عشوائي ثم رفض العناوين العالية. ويبقى identity map لأول 4GiB لأن هذا هو نطاق Linux handoff المثبت حالياً.

## TCG2 witness

يحتوي debug log في `make test-tcg2` على:

```text
TCG2 measured Linux kernel into PCR4
TCG2 measured Linux initrd into PCR4
```

ويقرأ initramfs داخل Linux الملف `/sys/class/tpm/tpm0/pcr-sha256/4` ويشترط قيمة غير صفرية. كما يثبت الاختبار السلبي أن config `measure=required` يوقف XBoot قبل Linux عند غياب TCG2 protocol. غياب TPM لا يمنع الإقلاع في config optional.

## Secure Boot witness

يوقع `make test-secureboot` XBoot وUbuntu kernel بشهادة OVMF SnakeOil الاختبارية، ويتحقق من التوقيع بواسطة `sbverify`، ثم يقلع OVMF في Secure Boot/SMM. يسجل direct path:

```text
Secure Boot enabled; authenticating Linux image through UEFI
Secure Boot firmware authentication passed
Starting Linux through direct 64-bit boot protocol
```

ويثبت serial `Linux version` و`Run /init as init process`. بعد ذلك يستخدم runner XBoot موقّعاً مع kernel غير موقّع، ويثبت `Secure Boot image authentication failed` وعدم ظهور `Linux version`. المصادقة هنا **delegated إلى firmware عبر LoadImage/UnloadImage**؛ ليست verifier Authenticode مستقلًا.

## Integrity and security boundary

يقفل `xboot-hash.conf` kernel/initrd المعروفين عبر SHA-256 allowlist، ويثبت الاختبار السلبي رفض byte معدل. هذه **سلامة artifact** وليست Secure Boot. لا يطبق XBoot حالياً إدارة PK/KEK/db/dbx، أو تحديثات authenticated variables، أو PE Authenticode/PKCS#7/X.509 verifier مستقلاً، أو anti-rollback policy.

## Interpretation

لدى XBoot مساران Linux مستقلان مثبتان: PE/COFF EFI-stub وdirect 64-bit Linux boot protocol. ولا يستدعي أي منهما GRUB أو U-Boot أو Limine أو systemd-boot أو rEFInd. الأدلة الحالية محدودة إلى assets المسجلة، OVMF/QEMU `q35`، TPM emulator، وOVMF SnakeOil test trust. ولا تبرر claim universal Linux compatibility أو hardware support.

هذا ليس claim certification. controller drivers لـ NVMe/AHCI/USB، BIOS، ARM64/RISC-V، physical-hardware matrix، production key enrollment، dbx revocation cases، LFN/كتابة FAT، fuzzing الشامل، وindependent laboratory certification تبقى غير مثبتة.

## References

1. [The Linux/x86 Boot Protocol](https://docs.kernel.org/arch/x86/boot.html)
2. [The Linux EFI Boot Stub](https://docs.kernel.org/admin-guide/efi-stub.html)
3. [UEFI Secure Boot and Driver Signing](https://uefi.org/specs/UEFI/2.10/32_Secure_Boot_and_Driver_Signing.html)
4. [UEFI Runtime Services](https://uefi.org/specs/UEFI/2.10/08_Services_Runtime_Services.html)
5. [TCG EFI Protocol Specification](https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/)
