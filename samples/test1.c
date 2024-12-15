call main:
hlt

jmp skip_test:
test:
push [bx + 0]
push 10
add
pop ax
ret
skip_test:
jmp skip_main:
main:
push 10
pop [bx + 1] ;pop to huy
push bx ;saving bx
push [bx + 1]
push 5
div
push bx
push bx
push 2
add
pop bx
pop [bx + 0]
call test:
pop bx
push ax
pop ax
ret
skip_main:
