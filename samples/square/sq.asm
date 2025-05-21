;setting bx value to global variables number
	push 0
	pop bx

;calling main
	call main:
	push ax
	out
	hlt

jmp skip_diskriminant:
diskriminant:
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
skip_diskriminant:

jmp skip_reshi_pozhaluysta_lineinoe:
reshi_pozhaluysta_lineinoe:
        ;if condition
                push [bx + 0] ;b
        

        push 0
        je skip_if_0:
        ;if body
                push 0
                push [bx + 1] ;c
                push [bx + 0] ;b
                div
                sub
                out
                push 1
                pop ax
                ret
        skip_if_0:
        

        ;if condition
                push [bx + 1] ;c
        

        push 0
        je skip_if_1:
        ;if body
                push 0
                pop ax
                ret
        skip_if_1:
        

        push 1488
        out
        push 211
        pop ax
        ret
skip_reshi_pozhaluysta_lineinoe:

jmp skip_reshi_pozhaluysta_kvadratnoe:
reshi_pozhaluysta_kvadratnoe:
        ;assignment to D
                ;START CALLING diskriminant
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
                

                call diskriminant:
                

                ;resetting bx
                pop bx
                ;pushing return value to stack
                push ax

                pop [bx + 3] ;D
        ;assignment to koren
                push [bx + 3] ;D
                sqrt
                pop [bx + 4] ;koren
        push 0
        push [bx + 1] ;b
        sub
        push [bx + 4] ;koren
        sub
        push 2
        push [bx + 0] ;a
        mul
        div
        out
        push 0
        push [bx + 1] ;b
        sub
        push [bx + 4] ;koren
        add
        push 2
        push [bx + 0] ;a
        mul
        div
        out
        push 0
        pop ax
        ret
skip_reshi_pozhaluysta_kvadratnoe:

jmp skip_reshi_pozhaluysta_uravneniye:
reshi_pozhaluysta_uravneniye:
        ;if condition
                push [bx + 0] ;a
        

        push 0
        je skip_if_2:
        ;if body
                ;START CALLING reshi_pozhaluysta_kvadratnoe
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
                

                call reshi_pozhaluysta_kvadratnoe:
                

                ;resetting bx
                pop bx
                ;pushing return value to stack
                push ax

                pop ax
                ret
        skip_if_2:
        

        ;START CALLING reshi_pozhaluysta_lineinoe
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
        

        call reshi_pozhaluysta_lineinoe:
        

        ;resetting bx
        pop bx
        ;pushing return value to stack
        push ax

        pop ax
        ret
skip_reshi_pozhaluysta_uravneniye:

jmp skip_main:
main:
        ;assignment to a
                push 0
                pop [bx + 0] ;a
        ;assignment to b
                push 0
                pop [bx + 1] ;b
        ;assignment to c
                push 0
                pop [bx + 2] ;c
        in
        pop [bx + 0] ;a
        in
        pop [bx + 1] ;b
        in
        pop [bx + 2] ;c
        ;assignment to num
                ;START CALLING reshi_pozhaluysta_uravneniye
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
                

                call reshi_pozhaluysta_uravneniye:
                

                ;resetting bx
                pop bx
                ;pushing return value to stack
                push ax

                pop [bx + 3] ;num
        push [bx + 3] ;num
        pop ax
        ret
skip_main:

