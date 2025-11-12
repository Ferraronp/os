# Сборка
```shell
# Очистка
rm -f *.o *.bin *.img

# Компиляция загрузчика
nasm -f bin boot.asm -o boot.bin

# Компиляция точки входа
nasm -f elf32 kernel_entry.asm -o kernel_entry.o

# Компиляция ядра
gcc -m16 -ffreestanding -fno-pie -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -O0 -c kernel.c -o kernel.o

# Линковка
ld -m elf_i386 -T linker.ld --oformat=binary -o kernel.bin kernel_entry.o kernel.o

# Создание образа
dd if=/dev/zero of=os.img bs=512 count=2880
dd if=boot.bin of=os.img conv=notrunc
dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc

# Запуск
qemu-system-i386 -fda os.img
```