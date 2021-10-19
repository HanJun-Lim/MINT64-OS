all: BootLoader.bin

# make BootLoader.bin by building BootLoader.asm using nasm compiler
BootLoader.bin: BootLoaderUSB.asm
	nasm -o BootLoader.bin BootLoaderUSB.asm

clean:
	rm -f BootLoader.bin
