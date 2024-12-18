;global variables setting
;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;function compilation
;compiling solve
jmp skip_solve:
solve:
        push [bx + 1]
        push 2
        pow
        push 4
        push [bx + 0]
        mul
        push [bx + 2]
        mul
        sub
        pop [bx + 3]    ;'diskriminant'
        push [bx + 3]
        push 0
        ja _cmp_t_0:
        push 0
        jmp _cmp_t_end_0:

        _cmp_t_0:
        push 1
        _cmp_t_end_0:

        push 0
        je skip_if_1:
                push [bx + 3]
                push 0.5
                pow
                pop [bx + 4]    ;'koren'
                push 0
                push 1
                sub
                push [bx + 1]
                mul
                push [bx + 4]
                sub
                push 2
                push [bx + 0]
                mul
                div
                out
                push 0
                push 1
                sub
                push [bx + 1]
                mul
                push [bx + 4]
                add
                push 2
                push [bx + 0]
                mul
                div
                out
                push 2
                pop ax
                ret
        skip_if_1:
        push [bx + 3]
        push 0
        jb _cmp_t_2:
        push 0
        jmp _cmp_t_end_2:

        _cmp_t_2:
        push 1
        _cmp_t_end_2:

        push 0
        je skip_if_3:
                push 0
                pop ax
                ret
        skip_if_3:
        push 0
        push 1
        sub
        push [bx + 1]
        mul
        push 2
        push [bx + 0]
        mul
        div
        out
        push 1
        pop ax
        ret
skip_solve:

;compiling main
jmp skip_main:
main:
        in
        pop [bx + 0]    ;'a'
        in
        pop [bx + 1]    ;'b'
        in
        pop [bx + 2]    ;'c'
        ;saving BX
        push bx

        ;function parameters
                push [bx + 0]
                push [bx + 1]
                push [bx + 2]
        

        ;incrementing bx
        push bx
        push 3
        add
        pop bx

        ;pushing arguments to function
        pop [bx + 2]
        pop [bx + 1]
        pop [bx + 0]
        call solve:
        ;resetting bx
        pop bx
        ;pushing return value to stack
        push ax

        pop [bx + 3]    ;'num'
        push [bx + 3]
        pop ax
        ret
skip_main:

