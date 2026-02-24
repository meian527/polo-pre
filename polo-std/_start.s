.intel_syntax noprefix
.globl main

.extern GetCommandLineW
.extern CommandLineToArgvW
.extern GetEnvironmentStringsW
.extern ExitProcess
main:
    push rbp
    mov rbp, rsp
    sub rsp, 0
    mov [rbp - 8], rcx
    mov [rbp - 16], rdx
    leave
    ret

.extern _start
