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
;compiling factorial
jmp skip_factorial:
factorial:
        push [bx + 0]
        push 1
        sub
        push 0
        ja comp_false_0:
        push 1
        jmp comp_false_end_0:

        comp_false_0:
        push 0
        comp_false_end_0:

        push 0
        je skip_if_1:
                push 1
                pop ax
                ret
        skip_if_1:
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

        ;pushing arguments to function memory
        pop [bx + 0]
        call factorial:
        pop bx
        push ax

        mul
        pop ax
        ret
skip_factorial:

;compiling main
jmp skip_main:
main:
        ;saving BX
        push bx

        ;function parameters
                push 10


        ;incrementing bx
        push bx
        push 0
        add
        pop bx

        ;pushing arguments to function memory
        pop [bx + 0]
        call factorial:
        pop bx
        push ax

        pop ax
        ret
skip_main:

