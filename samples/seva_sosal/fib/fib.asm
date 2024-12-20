push 5
pop [0] ;counter
;setting bx value to global variables number
	push 1
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
        pop [bx + 0] ;a
        push 1
        pop [bx + 1] ;b
        push [bx + 0] ;a
        out
        push [bx + 1] ;b
        out
        _while_start_0:
        push [0] ;counter
        push 0
        je _skip_while_0:
                push [bx + 1] ;b
                pop [bx + 2] ;tmp
                push [bx + 0] ;a
                push [bx + 1] ;b
                add
                pop [bx + 1] ;b
                push [bx + 2] ;tmp
                pop [bx + 0] ;a
                push [bx + 1] ;b
                out
                push [0] ;counter
                push 1
                sub
                pop [0] ;counter
        jmp _while_start_0:
        _skip_while_0:
        push 0
        pop ax
        ret
skip_main:

