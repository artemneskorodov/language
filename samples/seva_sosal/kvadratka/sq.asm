;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;compiling solve_linear
jmp skip_solve_linear:
solve_linear:
        push [bx + 1] ;c
        push 0
        je skip_if_0:
                push 0
                push [bx + 0] ;b
                sub
                push [bx + 1] ;c
                div
                pop [bx + 2] ;x
                push 0
                pop ax
                ret
        skip_if_0:
        hlt
skip_solve_linear:

;compiling discriminant
jmp skip_discriminant:
discriminant:
        push [bx + 1] ;b
        push [bx + 1] ;b
        mul
        push 4
        push [bx + 0] ;a
        mul
        push [bx + 2] ;c
        mul
        sub
        pop ax
        ret
skip_discriminant:

;compiling solve_quadratic_equation
jmp skip_solve_quadratic_equation:
solve_quadratic_equation:
        push [bx + 0] ;a
        push 0
        je skip_if_1:
                ;saving BX
                push bx

                ;function parameters
                        push [bx + 0] ;a
                        push [bx + 1] ;b
                        push [bx + 2] ;c
                

                ;incrementing bx
                push bx
                push 3
                add
                pop bx

                ;pushing arguments to function
                pop [bx + 2]
                pop [bx + 1]
                pop [bx + 0]
                call discriminant:
                ;resetting bx
                pop bx
                ;pushing return value to stack
                push ax

                pop [bx + 3] ;D
                push 0
                push [bx + 1] ;b
                sub
                push [bx + 3] ;D
                sqrt
                add
                push 2
                push [bx + 0] ;a
                mul
                div
                pop [bx + 4] ;x_one
                push 0
                push [bx + 1] ;b
                sub
                push [bx + 3] ;D
                sqrt
                sub
                push 2
                push [bx + 0] ;a
                mul
                div
                pop [bx + 5] ;x_two
                push [bx + 4] ;x_one
                out
                push [bx + 5] ;x_two
                out
                push 0
                pop ax
                ret
        skip_if_1:
        ;saving BX
        push bx

        ;function parameters
                push [bx + 1] ;b
                push [bx + 2] ;c
        

        ;incrementing bx
        push bx
        push 3
        add
        pop bx

        ;pushing arguments to function
        pop [bx + 1]
        pop [bx + 0]
        call solve_linear:
        ;resetting bx
        pop bx
        ;pushing return value to stack
        push ax

        pop [bx + 3] ;x
        push [bx + 3] ;x
        out
        push 0
        pop ax
        ret
skip_solve_quadratic_equation:

;compiling main
jmp skip_main:
main:
        push 0
        pop [bx + 0] ;a
        push 0
        pop [bx + 1] ;b
        push 0
        pop [bx + 2] ;c
        in
        pop [bx + 0] ;a
        in
        pop [bx + 1] ;b
        in
        pop [bx + 2] ;c
        ;saving BX
        push bx

        ;function parameters
                push [bx + 0] ;a
                push [bx + 1] ;b
                push [bx + 2] ;c
        

        ;incrementing bx
        push bx
        push 3
        add
        pop bx

        ;pushing arguments to function
        pop [bx + 2]
        pop [bx + 1]
        pop [bx + 0]
        call solve_quadratic_equation:
        ;resetting bx
        pop bx
        ;pushing return value to stack
        push ax

        pop [bx + 3] ;status
        push 0
        pop ax
        ret
skip_main:

