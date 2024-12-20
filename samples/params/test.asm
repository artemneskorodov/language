;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

jmp skip_main:
main:
        ;assignment to huy
                push 4
                sin
                pop [bx + 0] ;huy
        push [bx + 0] ;huy
        cos
        out
        push [bx + 0] ;huy
        sqrt
        out
        push 0
        pop ax
        ret
skip_main:

