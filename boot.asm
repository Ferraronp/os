bits 16
org 0x7C00

start:
    ; Настройка сегментов
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Очистка экрана
    mov ax, 0x0003
    int 0x10

    ; Загрузка ядра
    mov ah, 0x02
    mov al, 20      ; Увеличим количество секторов
    mov ch, 0       ; цилиндр
    mov cl, 2       ; сектор
    mov dh, 0       ; голова
    mov bx, 0x7E00  ; адрес загрузки
    int 0x13

    ; Переход к ядру
    jmp 0x0000:0x7E00

times 510-($-$$) db 0
dw 0xAA55