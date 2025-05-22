;assignment to counter
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

jmp skip_factorial:
factorial:
        ;assignment to unused
                push 28
                pop [bx + 1] ;unused
        ;assignment to unused_
                push 0
                pop [bx + 2] ;unused_
        ;assignment to counter
                push [0] ;counter
                push 1
                add
                pop [0] ;counter
        ;if condition
                push [bx + 0] ;nomer
        

        push 0
        je skip_if_0:
        ;if body
                ;START CALLING factorial
                ;saving BX
                push bx

                ;function parameters
                        push [bx + 0] ;nomer
                        push 1
                        sub
                

                ;incrementing bx
                push bx
                push 3
                add
                pop bx

                ;pushing arguments to function
                pop [bx + 0]
                

                call factorial:
                

                ;resetting bx
                pop bx
                ;pushing return value to stack
                push ax

                push [bx + 0] ;nomer
                mul
                pop ax
                ret
        skip_if_0:
        

        push 1
        pop ax
        ret
skip_factorial:

jmp skip_main:
main:
        ;assignment to nomer
                push 0
                pop [bx + 0] ;nomer
        in
        pop [bx + 0] ;nomer
        ;assignment to fact
                ;START CALLING factorial
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

