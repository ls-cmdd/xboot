# Secure Boot findings

## Sources

1. [UEFI 2.10 Secure Boot and Driver Signing](https://uefi.org/specs/UEFI/2.10/32_Secure_Boot_and_Driver_Signing.html)
2. [UEFI 2.10 Runtime Services](https://uefi.org/specs/UEFI/2.10/08_Services_Runtime_Services.html)
3. [Linux EFI Boot Stub](https://docs.kernel.org/admin-guide/efi-stub.html)
4. [Linux x86 boot protocol](https://docs.kernel.org/arch/x86/boot.html)

## Relevant conclusions

UEFI Secure Boot is a firmware image-authentication policy. The official specification distinguishes authenticated configuration data and image authentication; a local SHA-256 allowlist is not equivalent to PK/KEK/db/dbx certificate-chain validation. A PE/COFF image carries an embedded certificate table, and firmware applies its enrolled image-security policy when loading an EFI image.

XBoot therefore keeps two separate security layers. The first is a freestanding SHA-256 digest API and optional artifact allowlist. The second delegates image authentication to UEFI firmware through `LoadImage()` and immediately calls `UnloadImage()` before a direct Linux handoff. This prevents the direct raw path from bypassing firmware image-authentication policy when `SecureBoot=1`; XBoot does not claim to be an independent Authenticode/PKCS#7/X.509 verifier.

## Implementation

The minimal UEFI ABI now includes the correctly laid-out `EFI_RUNTIME_SERVICES` prefix through `GetVariable()`, the EFI Global Variable GUID, and the `SecureBoot` variable read. The runtime policy supports:

| Policy | Behaviour |
|---|---|
| `secure_boot=optional` or absent | EFI-stub follows firmware policy normally; direct path authenticates the Linux PE/COFF image whenever firmware reports `SecureBoot=1` |
| `secure_boot=required` | XBoot rejects the entry unless `SecureBoot` is readable and non-zero; direct path then performs the firmware `LoadImage`/`UnloadImage` authentication check |

The authentication helper builds the same parent-device-relative File Device Path used by the EFI-stub starter. It maps `EFI_SECURITY_VIOLATION` to `XBOOT_SECURITY_ERROR`, frees no image after a failed load because firmware did not return one, and requires `UnloadImage()` after a successful authentication-only load.

## QEMU witness

`make test-secureboot` creates a signed XBoot and signed Ubuntu kernel using the OVMF distribution's SnakeOil test certificate, verifies both with `sbverify`, and boots OVMF Secure Boot with SMM enabled and the enrolled SnakeOil variable store. The positive witness proves:

```text
Secure Boot enabled; authenticating Linux image through UEFI
Secure Boot firmware authentication passed
Starting Linux through direct 64-bit boot protocol
Linux version 6.8.0-138-generic
Run /init as init process
```

The same runner then builds an ESP with the signed XBoot but an unsigned kernel. It proves `Secure Boot image authentication failed` and asserts that no `Linux version` appears. Host tests also cover successful `LoadImage`/`UnloadImage` delegation and `EFI_SECURITY_VIOLATION` mapping.

## Boundary and limitations

This is a **firmware-delegated Secure Boot enforcement path**, not a standalone implementation of the full Secure Boot cryptographic stack. XBoot does not yet parse or manage PK/KEK/db/dbx, generate authenticated variable updates, evaluate revocation lists independently, or implement PE Authenticode certificate-chain verification itself. The positive evidence uses OVMF's SnakeOil test trust store, not a production OEM or Microsoft trust chain.

The SHA-256 allowlist remains artifact integrity and is not Secure Boot. TCG2 measurement is a separate measured-boot mechanism; it does not replace image authentication. Physical hardware, multiple firmware vendors, production key enrollment, dbx revocation cases, and external certification remain unverified.
