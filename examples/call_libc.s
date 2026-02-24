.intel_syntax noprefix
.globl main

.extern puts
.extern ExitProcess
main:
    push rbp
    mov rbp, rsp
    sub rsp, 0
    # string: Hello World
    lea rax, [rip + .L_str_0]
    mov rcx, rax
    mov rdx, rdx
    call puts
    mov rax, 0
    leave
    ret


.section .rodata
.L_str_0:
    .string "Hello World"

