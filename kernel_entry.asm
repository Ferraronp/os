bits 16
section .text

global _start
_start:
    ; Вызов главной функции C
    extern kernel_main
    call kernel_main

    ; Бесконечный цикл
    cli
    hlt