global _start
section .text
_start:
call main
mov rdi, rax
mov rax, 0x3c
syscall 
main:
push rbp
mov rbp, rsp
sub rsp, 0x10
mov rax, 0x402e000000000000
push rax
pop qword [rbp - 0x8]
mov rax, 0x0
push rax
pop qword [rbp - 0x10]
mov rax, 0x0
push rax
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
add rsp, 0x10
pop rbp
ret 
section .data
