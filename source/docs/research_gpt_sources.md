# GPT parser research notes

## Source reviewed

UEFI Specification 2.10 Chapter 5: https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html

## Required validation

The primary GPT header is at LBA 1 and the backup header at the device's last LBA. A GPT header must have the `EFI PART` signature, revision 1.0, a header size of at least 92 and no larger than the logical block size, a zero reserved field, a matching `MyLBA`, and a valid header CRC computed with the CRC field zeroed. The partition-entry array has its own CRC and its byte range must remain inside the device. Entries define inclusive starting and ending LBAs and must remain within FirstUsableLBA..LastUsableLBA without overlap. Unused entries have a zero partition-type GUID.

## XBoot scope

The first GPT milestone is read-only validation and enumeration. XBoot will not repair or write primary/backup GPT metadata. It will reject arithmetic overflow, invalid entry sizes/counts, out-of-device arrays, invalid CRCs, reversed or overlapping partitions, and partition ranges outside the usable area.
