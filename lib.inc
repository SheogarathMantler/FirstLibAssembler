; arguments - rdi, rsi, rdx

section .text

%define SYS_READ 0
%define SYS_WRITE 1
%define SYS_EXIT 60
%define stdin 0
%define stdout 1
%define stderr 2
; Принимает код возврата и завершает текущий процесс   - сделано
exit: 
    mov     rax, 60          ; 'exit' syscall number
    syscall 
; Принимает указатель на нуль-терминированную строку, возвращает её длину    - сделано
string_length:
    xor rax, rax                      
	.loop: cmp byte [rdi+rax], 0     ; rdi = адрес строки
               je .end
               inc rax
               jmp .loop
            .end:  ret	                       

; rax - length
; Принимает указатель на нуль-терминированную строку, выводит её в stdout    - сделано
print_string:
  call string_length 
  mov rdx, rax       ; length from rax to rdx
  mov rsi, rdi       ; "string" to rsi
  mov rax, SYS_WRITE ; 'write' syscall identifier
  mov rdi, stdout    ; stdout file descriptor
  syscall 
  ret
; Переводит строку (выводит символ с кодом 0xA)         - сделано
print_newline:                                   ; COROUTINE
  mov rdi, '\n'
; Принимает код символа и выводит его в stdout          - сделано
print_char:
  push rdi           ;save rdi
  mov rsi, rsp       ;top element adress
  mov rdx, 1         ; char - 1 byte
  mov rax, SYS_WRITE ; 'write' syscall identifier
  mov rdi, stdout    ; stdout file descriptor
  syscall
  pop rdi
  ret



; Выводит знаковое 8-байтовое число в десятичном формате    - сделано
print_int:
  test rdi, rdi
  jns print_uint    ; if number > 0 then print uint (jns - not sign)
  push rdi
  mov rdi, "-"
  call print_char   ; print "-"
  pop rdi
  neg rdi           ; convert to positive number and => print uint

; Выводит беззнаковое 8-байтовое число в десятичном формате       - сделано
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.

print_uint:            ; Coroutine with print_int
  xor rax, rax
  times 11 push word 0 ; 20 bytes + "0"
  mov rax,rdi          ; rax - number
  mov r8,10            ; for calculation
  mov r9, rsp          ; stack adress
  add rsp, 21
  .loop:
    xor rdx, rdx      
    dec rsp           ; move pointer
    div r8            ; rdx - res 
    add rdx, "0"      ; convert to ascii
    mov byte [rsp],dl ; move symbol to the stack
    test rax,rax      ; if not 0 repeat
    jnz .loop 
  mov rdi, rsp
  call print_string
  mov rsp, r9
  add rsp, 22         
  ret


; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
  xor rax, rax               ; count pointer
  .loop:
    mov r8b, byte [rdi+rax]  ; char from the first string to r8b
    mov r9b, byte[rsi+rax]   ; char from the second string to r9b
    cmp r8b, r9b
    jne .false               ; chars are not equal
    test r8b, r8b            ; if end of string and !false then true
    jz .true
    inc rax                  ; count pointer +1
    jmp .loop                ; next iteration
  .false: 
    mov rax, 0               ; not equal result
    ret
  .true:
    mov rax, 1               ; equal result
    ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока  - сделано
read_char:
  push 0
  mov rdi, stdin  ; stdin descriptor
  mov rsi, rsp    ; char's pointer
  mov rdx, 1      ; char - 1 byte
  xor rax, rax
  syscall         ; call read
  mov rax, [rsp]  ; char to rax
  add rsp,8       ; shifting for 8 bits
  ret
; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор              - сделано

read_word:                   ; rdi - buf addr, rsi = length
  xor rdx, rdx
  xor rax, rax
  .loop:  
    push rdi   
    push rsi
    push rdx                 ; save  addr, length and pointer
    call read_char
    pop rdx
    pop rsi
    pop rdi
    cmp rax, 0               ; if end of the string
    je .return
    cmp rdx, 0               ; if not the start of the string => read
    jne .read
    cmp rax, 0x9             ; if tab => continue
    je .loop 
    cmp rax, '\n'             ; if \n => continue
    je .loop
    cmp rax, ' '            ; if space => continue
    je .loop  
  .read:
    cmp rdx, rsi             ; end of buffer, current pointer = length => fail
    je .fail
    cmp rax, 0x9             ; if tab => return
    je .return
    cmp rax, '\n'             ; if \n => return
    je .return
    cmp rax, ' '            ; if space => return
    je .return
    mov [rdi+rdx], rax      ; char to buffer
    inc rdx                 ; current pointer += 1
    jmp .loop
  .fail:                   ; fail => result rax = 0
    xor rax, rax
    ret
  .return:
    xor rax, rax
    mov byte[rdi+rdx], 0    ; add terminator
    mov rax, rdi            ; not fail => result - buffer addr
    ret
  
; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось                       - сделано
parse_uint:
  xor rax, rax
  xor rdx, rdx
  xor rcx, rcx               ; counter
  xor r9, r9
  xor r10, r10
  mov r9, 10                 ; base = 10
  mov r10, "9"               ; max digit
  xor r11, r11               ; buffer
  .loop:
    mov r11b, byte[rdi, rcx]
    cmp r11b, "0"
    js .final
    cmp r10b, r11b
    js .final
    sub r11, "0"
    mul r9
    add rax, r11
    inc rcx                  ; counter += 1
    test rax, rax            ; rax != 0 => continue
    jnz .loop
    jz .final                ; rax == 0 => end of the number
  .final:
    mov rdx, rcx
    ret

; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось                               - сделано
parse_int:
  xor rax, rax
  push r12
  xor r12, r12
  cmp rdi, 0          ; end of the string
  je .end
  cmp byte[rdi],"-"
  jne .inverse        ; if there is sign
  mov r12, -1
  inc rdi
  .inverse:
    call parse_uint
    xor rax, r12 
    sub rax, r12      ; digit
    sub rdx, r12      ; length
    pop r12
    ret
  .end:
    ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0                 - сделано
; rdx = length, rsi - buffer addr, rdi - string addr
string_copy:
  xor rax, rax
  xor rcx, rcx           ; pointer
  push r12               ; save r12
  .copy:
    cmp rcx, rdx         ; if one doesn't fit the buffer, pointer == length
    je .fail
    mov r12, [rdi+rcx]   ; move char from the string to r12
    mov [rsi+rcx], r12   ; move char from r12 to buffer
    test r12, r12        ; if end of the string
    je .exit
    inc rcx              ; pointer += 1
    jmp .copy
  .fail:                 ; if fail => result = 0 
    mov rax, 0
    pop r12
    ret
  .exit:
    pop r12
    mov rax, rcx         ;  result = length (from rcx to rax)
    ret
