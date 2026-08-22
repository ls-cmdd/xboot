# Storage abstraction research notes

## Source reviewed

The UEFI 2.10 Media Access specification at https://uefi.org/specs/UEFI/2.10/13_Protocols_Media_Access.html defines Block I/O as the firmware-facing media interface. The media description supplies a media ID, block size, and last block; reads operate in whole blocks. The specification also describes FAT system partitions and a partition discovery order beginning with GPT, then optical formats, then legacy MBR.

## Implementation implications

XBoot's storage core will expose a read-only block-device contract with explicit `block_size`, `last_block`, and `media_id`. All byte reads must be translated to whole-block reads with checked multiplication and range validation. A UEFI Block I/O adapter can then implement the contract without exposing firmware protocol details to VFS or filesystem parsers. Partition discovery and filesystem mounting remain separate layers; a successful block read must never be reported as a mounted filesystem.
