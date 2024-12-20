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
                pop [bx + 0] ;huy
        push -604798
        out
        push [bx + 0] ;huy
        pop ax
        ret
skip_main:

