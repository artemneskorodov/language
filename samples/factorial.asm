;global variables setting
push 0
pop [0]    ;'counter'
;setting bx value to global variables number
	push 1
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;function compilation
;compiling factorial
jmp skip_factorial:
factorial:
        push [0]
        push 1
        add
        pop [0]    ;'counter'
        push [bx + 0]
        push 0
        ja _cmp_t_0:
        push 0
        jmp _cmp_t_end_0:

        _cmp_t_0:
        push 1
        _cmp_t_end_0:

        push 0
        je skip_if_1:
                push [bx + 0]
                ;saving BX
                push bx

                ;function parameters
                        push [bx + 0]
                        push 1
                        sub
                

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

                mul
                pop ax
                ret
        skip_if_1:
        push 1
        pop ax
        ret
skip_factorial:

;compiling main
jmp skip_main:
main:
        in
        pop [bx + 0]    ;'nomer'
        ;saving BX
        push bx

        ;function parameters
                push [bx + 0]
        

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

        pop [bx + 1]    ;'fact'
        push [bx + 1]
        out
        push [0]
        out
        push [bx + 1]
        pop ax
        ret
skip_main:

