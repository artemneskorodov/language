;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;compiling factorial
jmp skip_factorial:
factorial:
        push 1
        pop [bx + 1] ;otvet
        _while_start_0:
        push [bx + 0] ;nomer
        push 0
        ja _cmp_t_1:

        push 0
        jmp _cmp_t_end_1:

        _cmp_t_1:
        push 1
        _cmp_t_end_1:

        push 0
        je _skip_while_0:
                push [bx + 1] ;otvet
                push [bx + 0] ;nomer
                mul
                pop [bx + 1] ;otvet
                push [bx + 0] ;nomer
                push 1
                sub
                pop [bx + 0] ;nomer
        jmp _while_start_0:
        _skip_while_0:
        push [bx + 1] ;otvet
        pop ax
        ret
skip_factorial:

;compiling main
jmp skip_main:
main:
        in
        pop [bx + 0] ;nomer
        ;saving BX
        push bx

        ;function parameters
                push [bx + 0] ;nomer


        ;incrementing bx
        push bx
        push 1
        add
        pop bx

        ;pushing arguments to function
        pop [bx + 0]
        call factorial:
        ;resetting bx
        pop bx
        ;pushing return value to stack
        push ax

        pop ax
        ret
skip_main:

