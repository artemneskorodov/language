push 0
pop [0] ;counter
;setting bx value to global variables number
	push 1
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

;compiling factorial
jmp skip_factorial:
factorial:
        push [0] ;counter
        push 1
        add
        pop [0] ;counter
        push [bx + 0] ;nomer
        push 0
        je skip_if_0:
                push [bx + 0] ;nomer
                ;saving BX
                push bx

                ;function parameters
                        push [bx + 0] ;nomer
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
        push 0
        pop [bx + 0] ;nomer
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

        pop [bx + 1] ;fact
        push [bx + 1] ;fact
        out
        push [0] ;counter
        out
        push [bx + 1] ;fact
        pop ax
        ret
skip_main:

