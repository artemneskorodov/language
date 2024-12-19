;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;compiling main
jmp skip_main:
main:
        push 1
        pop [bx + 0] ;huy
        push 2
        out
        push 0
        pop ax
        ret
skip_main:

