section .text
_start:
call main
mov rdi, rax
mov rax, 0x3c
syscall 
diskriminant:
push rbp
mov rbp, rsp
sub rsp, 0x0
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq [rsp + 0x0], xmm0
mov rax, 0x4010000000000000
push rax
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x20]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
reshi_pozhaluysta_lineinoe:
push rbp
mov rbp, rsp
sub rsp, 0x0
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
xorpd xmm1, xmm1
cmpeqsd xmm0, xmm1
movq rax, xmm0
not rax
test rax, rax
jz .loc_00000000:
mov rax, 0x0
push rax
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
divsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
call std_out
mov rax, 0x3ff0000000000000
push rax
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
.loc_00000000
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
xorpd xmm1, xmm1
cmpeqsd xmm0, xmm1
movq rax, xmm0
not rax
test rax, rax
jz .loc_00000001:
mov rax, 0x0
push rax
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
.loc_00000001
mov rax, 0x4097400000000000
push rax
movq xmm0, [rsp + 0x0]
add rsp, 0x8
call std_out
mov rax, 0x406a600000000000
push rax
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
reshi_pozhaluysta_kvadratnoe:
push rbp
mov rbp, rsp
sub rsp, 0x10
movq xmm0, [rbp + 0x20]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
call diskriminant
add rsp, 0x18
sub rsp, 0x8
movq [rsp + 0x0], xmm0
pop [rbp - 0x8]
movq xmm0, [rbp - 0x8]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
xmm0, xmm0
movq [rsp + 0x0], xmm0
pop [rbp - 0x10]
mov rax, 0x0
push rax
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rbp - 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq [rsp + 0x0], xmm0
mov rax, 0x4000000000000000
push rax
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
divsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
call std_out
mov rax, 0x0
push rax
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
subsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rbp - 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
addsd xmm0, xmm1
movq [rsp + 0x0], xmm0
mov rax, 0x4000000000000000
push rax
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
mulsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x8]
movq xmm1, [rsp + 0x0]
add rsp, 0x8
divsd xmm0, xmm1
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
call std_out
mov rax, 0x0
push rax
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x10
pop rbp
ret 
reshi_pozhaluysta_uravneniye:
push rbp
mov rbp, rsp
sub rsp, 0x0
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
xorpd xmm1, xmm1
cmpeqsd xmm0, xmm1
movq rax, xmm0
not rax
test rax, rax
jz .loc_00000002:
movq xmm0, [rbp + 0x20]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
call reshi_pozhaluysta_kvadratnoe
add rsp, 0x18
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
.loc_00000002
movq xmm0, [rbp + 0x20]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp + 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
call reshi_pozhaluysta_lineinoe
add rsp, 0x10
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x0
pop rbp
ret 
main:
push rbp
mov rbp, rsp
sub rsp, 0x20
mov rax, 0x0
push rax
pop [rbp - 0x8]
mov rax, 0x0
push rax
pop [rbp - 0x10]
mov rax, 0x0
push rax
pop [rbp - 0x18]
call std_in
movq [rbp - 0x8], xmm0
call std_in
movq [rbp - 0x10], xmm0
call std_in
movq [rbp - 0x18], xmm0
movq xmm0, [rbp - 0x18]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp - 0x10]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rbp - 0x8]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
call reshi_pozhaluysta_uravneniye
add rsp, 0x18
sub rsp, 0x8
movq [rsp + 0x0], xmm0
pop [rbp - 0x20]
movq xmm0, [rbp - 0x20]
sub rsp, 0x8
movq [rsp + 0x0], xmm0
movq xmm0, [rsp + 0x0]
add rsp, 0x8
add rsp, 0x20
pop rbp
ret 
section .data
