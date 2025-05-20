global _start
section .text
_start:
call main
mov rdi, rax
mov rax, 0x3c
syscall
factorial:
push rbp
mov rbp, rsp
sub rsp, 0x8
movq xmm0, qword [counter]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
mov rax, 0x3ff0000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
pop qword [counter]
movq xmm0, qword [rbp + 0x10]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
mov rax, 0x3ff0000000000000
push rax
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
pop qword [rbp - 0x8]
movq xmm0, qword [rbp + 0x10]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
xorpd xmm1, xmm1
cmpeqsd xmm0, xmm1
movq rax, xmm0
not rax
test rax, rax
jz .loc_00000000
movq xmm0, qword [rbp - 0x8]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
call factorial
add rsp, 0x8
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rbp + 0x10]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rsp + 0x8]
movq xmm1, qword [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
add rsp, 0x8
pop rbp
ret
.loc_00000000:
mov rax, 0x3ff0000000000000
push rax
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
add rsp, 0x8
pop rbp
ret
main:
push rbp
mov rbp, rsp
sub rsp, 0x10
mov rax, 0x4014000000000000
push rax
pop qword [rbp - 0x8]
movq xmm0, qword [rbp - 0x8]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
call factorial
add rsp, 0x8
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
pop qword [rbp - 0x10]
movq xmm0, qword [rbp - 0x10]
sub rsp, 0x8
movq qword [rsp + 0x0], xmm0
movq xmm0, qword [rsp + 0x0]
add rsp, 0x8
add rsp, 0x10
pop rbp
ret
section .data
counter dq 0.0
