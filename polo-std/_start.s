.intel_syntax noprefix
.globl main

.extern GetCommandLineW
.extern CommandLineToArgvW
.extern GetEnvironmentStringsW
.extern ExitProcess
.extern _start
_start:
    push rbp
    mov rbp, rsp
    sub rsp, 40
    # declare var: hInstance
    mov rax, -1
    mov [rbp - 8], rax
    # declare var: argc
    mov rax, 0
    mov [rbp - 16], rax
    # declare var: argv
    call GetCommandLineW
    mov rcx, rax
    lea rax, [rbp - 16]
    mov rax, [rbp - 16]
    neg rax
    mov rdx, rax
    call CommandLineToArgvW
    mov [rbp - 24], rax
    # declare var: nShowCmd
    mov rax, 1
    mov [rbp - 32], rax
    mov rax, [rbp - 16]
    mov rcx, rax
    mov rax, [rbp - 24]
    mov rdx, rax
    call main
    mov rcx, rax
    call ExitProcess
    leave
    ret

