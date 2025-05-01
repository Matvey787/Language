# find . -maxdepth 1 -type f ! -name "*.ASM" ! -name "*.sh" ! -name "*.inc" -exec rm -f {} \;
nasm -f elf64 "$1.ASM" -o "$1.o"
ld -m elf_x86_64 -s -o "$1" "$1.o"
./"$1"
