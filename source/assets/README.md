# Local Linux test assets

يستخدم اختبار QEMU في XBoot ملفي kernel وinitrd حقيقيين من Ubuntu. لا تُضمَّن الملفات الثنائية الكبيرة في Git؛ يوفَّران محلياً عبر نسخ `/boot/vmlinuz-*` و`/boot/initrd.img-*` إلى هذا المجلد، أو عبر متغيري `LINUX_KERNEL` و`LINUX_INITRD` عند تشغيل Make.

الـ artifacts المستخدمة في خط الأساس الحالي هي:

| الملف | الحجم | SHA-256 |
|---|---:|---|
| `vmlinuz` | 15055240 bytes | `d74be574189057a036866309fd5546724a92390a4f6ba086e93ddab2f6e0e01d` |
| `initrd.img` | 66113568 bytes | `79c1188d6ca7fd06398b4ccd26615567047bfe47a21faaff84d79327b96aca06` |

هذه البصمات تثبت artifact المستخدم في جلسة الاختبار الحالية فقط؛ لا تمثل توقيعاً أمنياً أو provenance موزعاً من Ubuntu.
