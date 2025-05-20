use64

dd std_in
dd std_out

SYMBOLS_IN_FRAC    equ 6
BUFFER_SIZE        equ 512
NUMBER_BUFFER_SIZE equ 64

std_out:
    sub rsp, BUFFER_SIZE
    ;---------------------------------------------------------------------------
    ; Current writing symbol in RDI
    mov rdi, rsp
    ;---------------------------------------------------------------------------
    ; Checking sign bit
    movq rax, xmm0
    test rax, rax
    jns .not_negative
        ;-----------------------------------------------------------------------
        ; Printing -
        mov al, '-'
        stosb
        ;-----------------------------------------------------------------------
        ; XMM0 = 0 - XMM0
        xorpd xmm1, xmm1
        subsd xmm1, xmm0
        movq xmm0, xmm1
        ;-----------------------------------------------------------------------
    .not_negative:
    ;---------------------------------------------------------------------------
    ; Rounding value towards zeros, integer part of double is in R8
    movq xmm1, xmm0
    cvttsd2si r8, xmm1
    cvtsi2sd xmm1, r8
    ;---------------------------------------------------------------------------
    ; Fraction in XMM0
    subsd xmm0, xmm1
    ;---------------------------------------------------------------------------
    ; Muplyplying XMM0 by 10 SYMBOLS_IN_FRAC times
    mov rcx, SYMBOLS_IN_FRAC
    mov rax, 0x4024000000000000 ; 10
    movq xmm1, rax
    xor rax, rax ;//TODO
    .multiplying_loop:
        mulsd xmm0, xmm1
    loop .multiplying_loop
    ;---------------------------------------------------------------------------
    ; Rounding the fraction and translating it into and integer in stack
    roundsd xmm0, xmm0, 0
    cvttsd2si r9, xmm0
    ;---------------------------------------------------------------------------
    ; Printing integer part
    call print_integer
    ;---------------------------------------------------------------------------
    ; Checking if fraction is not zero
    test r9, r9
    jz .skip_fraction
        ;-----------------------------------------------------------------------
        ; Printing point and fraction
        mov al, '.'
        stosb
        mov r8, r9
        call print_integer
        ;-----------------------------------------------------------------------
    .skip_fraction:
    mov al, 0xA
    stosb
    ;---------------------------------------------------------------------------
    ; Write system call parameters
        ;-----------------------------------------------------------------------
        mov rsi, rsp        ; Buffer start in RSI
        mov rdx, rdi        ;
        sub rdx, rsi        ; Length of output in RDX
        mov rax, 1          ; write system call in RAX
        mov rdi, 1          ; Output file stream in RDI
        ;-----------------------------------------------------------------------
    syscall
    add rsp, BUFFER_SIZE
    ;---------------------------------------------------------------------------
    ret

; r8 - parameter
; rdi - buffer position
print_integer:
    ;---------------------------------------------------------------------------
    ; Saving RDI value in stack
    push rdi
    ;---------------------------------------------------------------------------
    ; Moving number buffer last symbol address in RDI
    sub rsp, NUMBER_BUFFER_SIZE
    lea rdi, [rsp + 64 - 1]
    ;---------------------------------------------------------------------------
    ; DF flag is set to 1 to write backwards
    std
    ;---------------------------------------------------------------------------
    ; Divider in EBX, counter of digits in RCX
    mov ebx, 10
    xor rcx, rcx
    ;---------------------------------------------------------------------------
    .loop_start:
        ;-----------------------------------------------------------------------
        ; Lower 32 bits of value in EAX and higher 32 bits in EDX
        mov eax, r8d
        mov rdx, r8
        shr rdx, 32
        ;-----------------------------------------------------------------------
        ; Dividing by 10
        div ebx
        ;-----------------------------------------------------------------------
        ; Saving new value in R8
        mov r8, rax
        ;-----------------------------------------------------------------------
        ; Digit symbol is printed to NumberBuffer and RDI which is pointer to
        ; current symbol in this buffer is decremented
        xor rax, rax
        mov al, dl
        add al, '0'
        stosb
        ;-----------------------------------------------------------------------
        ; Incrementing counter of digits
        inc rcx
        ;-----------------------------------------------------------------------
        ; Checking that 0 is not reached
        test r8, r8
        ;-----------------------------------------------------------------------
        jnz .loop_start
    ;---------------------------------------------------------------------------
    ; Setting DF = 0 to use stosb to write forward
    cld
    ;---------------------------------------------------------------------------
    ; Last written digit address in RDI
    lea rsi, [rdi + 1]
    ;---------------------------------------------------------------------------
    ; Resetting RDI which is pointer of current symbol in Buffer
    add rsp, NUMBER_BUFFER_SIZE
    pop rdi
    ;---------------------------------------------------------------------------
    ; Copying number from NumberBuffer to Buffer
    .loop_copy:
        lodsb
        stosb
        loop .loop_copy
    ;---------------------------------------------------------------------------
    ret
    ;---------------------------------------------------------------------------

std_in:
    sub rsp, BUFFER_SIZE
    ;---------------------------------------------------------------------------
    ; Parameters for read system call
        ;-----------------------------------------------------------------------
        mov rax, 0          ; sys_read
        mov rdi, 0          ; Descriptor of stdin
        mov rsi, rsp        ; Buffer in RSI
        mov rdx, 512        ; Buffer size
        ;-----------------------------------------------------------------------
    syscall
    ;---------------------------------------------------------------------------
    ; Length of read number is saved in R8 to check that end is not reached fast
    mov r8, rax
    add r8, rsp
    sub r8, 1
    ;---------------------------------------------------------------------------
    ; Buffer in RSI
    mov rsi, rsp
    ;---------------------------------------------------------------------------
    ; XMM0 will store result value
    xorpd xmm0, xmm0
    ;---------------------------------------------------------------------------
    ; Loading 10 in XMM1, it will be used as multiplier in one loop and
    ; divider in another
    mov rax, 0x4024000000000000 ; 10
    movq xmm1, rax
    ;---------------------------------------------------------------------------
    ; Checking for negative value
    xor rax, rax
    mov al, [rsi]
    cmp al, '-'
    mov rax, 0x3ff0000000000000 ; 1
    jne .skip_negative
        mov rax, 0xbff0000000000000 ; -1
        inc rsi
    .skip_negative:
    ;---------------------------------------------------------------------------
    ; Saving multiplier in XMM2, it is 1 if value is positive and -1 if negative
    movq xmm2, rax
    ;---------------------------------------------------------------------------
    ; Resetting RAX to zeros
    xor rax, rax
    ;---------------------------------------------------------------------------
    ; Reading integer part
    .loop_start1:
        ;-----------------------------------------------------------------------
        ; Checking that number end is not reached
        cmp rsi, r8
        je .number_end
        ;-----------------------------------------------------------------------
        ; Reading digit from Buffer
        lodsb
        ;-----------------------------------------------------------------------
        ; Checking that integer part end is not reached
        cmp al, '.'
        je .integer_end
        ;-----------------------------------------------------------------------
        ; Setting XMM3 to digit double value
        call DigitRAX2XMM3
        ;-----------------------------------------------------------------------
        ; Multiplying XMM0 by 10 and adding digit value
        mulsd xmm0, xmm1
        addsd xmm0, xmm3
        ;-----------------------------------------------------------------------
        jmp .loop_start1
    ;---------------------------------------------------------------------------
    .integer_end:
    ;---------------------------------------------------------------------------
    ; Loading 0.1 in XMM4, which it is used as multiplier for every digit in
    ; fraction loop
    mov rax, 0x3fb999999999999a
    movq xmm4, rax
    ;---------------------------------------------------------------------------
    ; Resetting RAX to zeros
    xor rax, rax
    ;---------------------------------------------------------------------------
    ; Reading number fraction
    .loop_start2:
        ;-----------------------------------------------------------------------
        ; Checking that number end is not reached
        cmp rsi, r8
        je .number_end
        ;-----------------------------------------------------------------------
        ; Reading digit to AL
        lodsb
        ;-----------------------------------------------------------------------
        ; Adding digit value multiplied by negative power of 10 to XMM0
        call DigitRAX2XMM3
        mulsd xmm3, xmm4 ; xmm5 = digit * 10^(-iteration)
        addsd xmm0, xmm3 ; xmm0 = xmm0 + xmm5
        ;-----------------------------------------------------------------------
        ; Decreasing power of 10, which we multiply digits values by
        divsd xmm4, xmm1
        ;-----------------------------------------------------------------------
        jmp .loop_start2
    ;---------------------------------------------------------------------------
    .number_end:
    ;---------------------------------------------------------------------------
    ; Multiplying XMM0 by sign value
    mulsd xmm0, xmm2
    ;---------------------------------------------------------------------------
    add rsp, BUFFER_SIZE
    ret
    ;---------------------------------------------------------------------------
    ; Digits values array
;    DigitsValue dq 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0
    ;---------------------------------------------------------------------------
DigitRAX2XMM3:
    mov rcx, 0x3ff0000000000000
    movq xmm5, rcx
    xorpd xmm3, xmm3
    mov rcx, rax
    sub rcx, '0'
    test rcx, rcx
    jz .skip_loop
    .loop_start:
        addpd xmm3, xmm5
        loop .loop_start
    .skip_loop:
    ret
