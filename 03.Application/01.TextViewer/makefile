################################################################################
# set build rules and environment
################################################################################

# *************** define Compiler, Linker, Library ***************

NASM64 = nasm -f elf64
GCC64 = x86_64-pc-linux-gcc.exe -c -m64 -ffreestanding -mcmodel=large -fno-common
LD64 = x86_64-pc-linux-ld.exe -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e _START -Ttext 0x0000
LDFLAGS = -lMINT64OS -L../../UserLibrary
CFLAGS = -I../../UserLibrary/Source


# *************** define OBJCOPY to make Application Program image ***************

OBJCOPY64 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss


# *************** define directory ***************

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source



################################################################################
# set build rules, how to build
################################################################################

all: prepare Output.elf


# *************** create directory for object file  ***************

prepare:
	mkdir -p $(OBJECTDIRECTORY)


# *************** create dependency information of C source file ***************

dep:
	@echo === Make Dependancy File ===
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependancy Search Complete ===


# *************** create dependency file, elf file ***************

ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile Main.elf


# *************** create IA-32e mode binary ***************

Output.elf: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECTDIRECTORY)/Main.elf $@
		

# *************** clean Temp dir ***************

clean:
	rm -f *.elf
	rm -f $(OBJECTDIRECTORY)/*.*



################################################################################
# re-called by Make, working dir is Temp
################################################################################

# *************** define C source file to build ***************

CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))


# *************** define how to make .c file to .o file ***************

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $< $(CFLAGS)


# *************** define how to make .asm file to .o file ***************

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<


# *************** create dependency-related file ***************

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep $(CFLAGS)


# *************** build Application Program elf file ***************

Main.elf: $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -r -o $@ $^ $(LDFLAGS)


# *************** if Dependency.dep exists, include it (to make) ***************

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
