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
        push [bx + 0] ;n
        push 0
        je skip_if_0:
                push [bx + 0] ;n
                ;saving BX
                push bx

                ;function parameters
                        push [bx + 0] ;n
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
        skip_if_0:
        push 1
        pop ax
        ret
skip_factorial:

;compiling main
jmp skip_main:
main:
        push 5
        pop [bx + 0] ;n
        in
        pop [bx + 0] ;n
        ;saving BX
        push bx

        ;function parameters
                push [bx + 0] ;n


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

        out

        pop ax
        ret
skip_main:

