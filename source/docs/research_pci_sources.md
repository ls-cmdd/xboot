# PCI discovery research notes

## Sources reviewed

1. Microsoft's PCI configuration-space documentation at https://learn.microsoft.com/en-us/windows-hardware/drivers/pci/accessing-pci-device-configuration-space explains that the PCI configuration header is governed by the PCI Local Bus specification, that extended configuration space may exceed 256 bytes, and that an MCFG table is one firmware path for memory-mapped configuration access.

2. Arm's PCI address-space documentation at https://developer.arm.com/documentation/dui0102/g/pci-management-library/pci-configuration/pci-address-spaces describes the three PCI address spaces and states that each device has a 256-byte configuration header containing device type, manufacturer, resource requirements, and class information. It also describes enumeration through configuration cycles.

## Implementation implications

XBoot will first implement a read-only generic configuration backend and a bounded enumerator. The enumerator will record vendor/device IDs, class/subclass/programming interface, header type, multifunction state, and BAR words without writing configuration registers. An actual UEFI/PCI access backend and resource assignment remain separate tasks; discovery evidence must not be confused with driver or device support.
