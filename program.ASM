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
	push 0
	pop rax
	mov [rbp - 4], eax
	; [comment] for has been started
	mov  dword [rbp - 8], 0
	jmp CL_for0

IL_for0:
	; [comment] for_body
	mov rax, [rbp - 4]
	push rax
	push 1
	pop rax
	pop rbx
	add rax, rbx
	push rax
	pop rax
	mov [rbp - 4], eax
	; [comment] for_step
	mov rax, [rbp - 8]
	push rax
	push 2
	pop rax
	pop rbx
	add rax, rbx
	push rax
	pop rax
	mov [rbp - 8], eax
	; [comment] for_condition
	
CL_for0:
	cmp dword [rbp - 8], 10
	jl IL_for0
	; [comment] for has been ended
	mov edi, [rbp - 4]
	call yopta
	FINISH
	
yopta:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov [rbp - 4], edi
	mov edi, [rbp - 4]
	call printNum
	mov rsp, rbp
	pop rbp
	ret
	