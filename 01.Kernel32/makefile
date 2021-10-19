########################################
# set build environment and rules
########################################

NASM32 = nasm
GCC32  = x86_64-pc-linux-gcc.exe -c -m32 -ffreestanding
LD32   = x86_64-pc-linux-ld.exe -melf_i386 -T ../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source



########################################
# set build contents and build rules
########################################

all: prepare Kernel32.bin

prepare:
	mkdir -p $(OBJECTDIRECTORY)

$(OBJECTDIRECTORY)/EntryPoint.bin: $(SOURCEDIRECTORY)/EntryPoint.s
	$(NASM32) -o $@ $<

dep:
	@echo === Make Dependency File ===
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile Kernel32.elf

$(OBJECTDIRECTORY)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJECTDIRECTORY)/Kernel32.elf $@

Kernel32.bin: $(OBJECTDIRECTORY)/EntryPoint.bin $(OBJECTDIRECTORY)/Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin
	rm -f $(OBJECTDIRECTORY)/*.*



################################################
# re-called by Make, based on Temp dir
################################################

CENTRYPOINTOBJECTFILE = Main.o
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(subst Main.o, , $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES))))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))



# define rule converting .c to .o

%.o: ../$(SOURCEDIRECTORY)/%.c
		$(GCC32) -c $<


# define rule converting .asm to .o

%.o: ../$(SOURCEDIRECTORY)/%.asm
		$(NASM32) -f elf32 -o $@ $<
	

InternalDependency:
	$(GCC32) -MM $(CSOURCEFILES) > Dependency.dep

Kernel32.elf: $(CENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD32) -o $@ $^


ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
