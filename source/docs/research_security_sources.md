# Security boundary research notes

## Sources reviewed

The UEFI 2.10 Secure Boot and Driver Signing specification at https://uefi.org/specs/UEFI/2.10/32_Secure_Boot_and_Driver_Signing.html describes authenticated image verification, signature databases, embedded PE/COFF certificates, and the distinction between image integrity hashing and public-key signature verification.

The TCG EFI Protocol specification at https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/ defines an EFI-facing TPM interface for presence, PCR banks, boot-log access, extending hashes to PCRs, and appending events. The TCG EFI Platform specification at https://trustedcomputinggroup.org/resource/tcg-efi-platform-specification/ covers measuring boot events into PCRs and recording them in the event log.

## Implementation implications

XBoot will expose a fail-closed security policy boundary. It may record Secure Boot and TPM observations and accept verifier/measurement callbacks, but it must return `UNSUPPORTED` when a required cryptographic verifier or TPM measurement backend is absent. A policy record is not a signature verifier, and a measured-boot callback is not proof that a TPM event was actually extended.
