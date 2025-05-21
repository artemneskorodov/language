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
mov rax, 0x4024000000000000
push rax
mov rax, 0x4014000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
pop qword [rbp - 0x8]
mov rax, 0x4024000000000000
push rax
mov rax, 0x4024000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x4014000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x408f400000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x4034000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x405ec00000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x405ec00000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rbp - 0x8]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
mov rax, 0x0
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
pop qword [rbp - 0x10]
mov rax, 0x0
push rax
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
add rsp, 0x10
pop rbp
ret 
section .data
