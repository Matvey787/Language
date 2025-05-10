section .data
    buffer times 16 db 0

section .text
global _start

; |-----------------------------|
; | Convert number to string    |
; | Entry: edi - number         |
; | Exit: rax - pointer to str, | 
; |       rdx - lenngth of str  |
; | Destr: ???                  |
; |-----------------------------|

number_to_string:
    push rbp
    mov rbp, rsp
    push rbx

    mov rbx, buffer + 15
    mov byte [rbx], 0
    mov eax, edi
    mov rcx, 10

.convert_loop:
    xor edx, edx
    div rcx
    add dl, '0'
    dec rbx
    mov [rbx], dl
    test eax, eax
    jnz .convert_loop

    mov rax, rbx
    mov rdx, buffer + 15
    sub rdx, rbx

    pop rbx
    mov rsp, rbp
    pop rbp
    ret

; |----------------------------------|
; | Print number                     |
; | Entry: edi - number (str format) |
; | Exit: nothing                    |
; | Destr: ???                       |
; |----------------------------------|

printNum:
    push rbp
    mov rbp, rsp
    push rbx

    mov ebx, edi
    call number_to_string

    mov rsi, rax
    mov rax, 1
    mov rdi, 1
    
    syscall

    pop rbx
    mov rsp, rbp
    pop rbp
    ret

%macro FINISH 0
    mov rax, 60
    xor rdi, rdi
    syscall
%endmacro

_start:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	push 3
	pop rax
	mov [rbp - 4], eax
	push 0
	pop rax
	mov [rbp - 8], eax
	push 4
	pop rax
	mov [rbp - 12], eax
	; [comment] while has been started
	jmp CL_WH0

IL_WH0:
	; [comment] while_body
	mov rax, [rbp - 4]
	push rax
	push 1
	pop rbx
	pop rax
	sub rax, rbx
	push rax
	pop rax
	mov [rbp - 4], eax
	; [comment] while_condition
	
CL_WH0:
	cmp [rbp - 4], 1
	jle END_WH0
	cmp [rbp - 12], 3
	jg END_WH0
	cmp [rbp - 8], 0
	jne IL_WH0
	
END_WH0:
	; [comment] while has been ended
	FINISH
	