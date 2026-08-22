.intel_syntax noprefix
.code64
.text

.globl xboot_linux_enter_64
xboot_linux_enter_64:
    cli
    mov r10, rcx
    mov r11, rdx
    mov r12, r9
    mov rax, r8
    mov cr3, rax
    lea rax, [rip + xboot_linux_gdt_descriptor]
    lgdt [rax]
    push 0x10
    lea rax, [rip + xboot_linux_entry64]
    push rax
    retfq

xboot_linux_entry64:
    mov ax, 0x18
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov rsp, r12
    mov rsi, r11
    xor ebp, ebp
    xor edi, edi
    xor ebx, ebx
    jmp r10

.p2align 3
xboot_linux_gdt:
    .quad 0x0000000000000000
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
xboot_linux_gdt_end:

xboot_linux_gdt_descriptor:
    .word xboot_linux_gdt_end - xboot_linux_gdt - 1
    .quad xboot_linux_gdt
