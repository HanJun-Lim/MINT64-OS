# for building library file
# 		make library file (libMINT64OS.a) by compiling files in Source dir


#####################################################
# set build rules, build environment
#####################################################

# ********** set options of Compiler, AR Program **********

NASM64 = nasm -f elf64

GCC64  = x86_64-pc-linux-gcc.exe -c -m64 -ffreestanding -mcmodel=large -fno-common
		# -mcmodel=large : create code available on upper 2 GB memory (large option)
		# -fno-common : create no common section

# AR program used to make static-link library
AR64   = x86_64-pc-linux-ar.exe rcs
		# r : replace object files that included in library file, or create new object file
		# c : create library file when library file not exists
		# s : create the index of symbol that exists in library file


# ********** define Directory location **********

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source



#####################################################
# set build items, how to build
#####################################################

all: prepare libMINT64OS


# ********** make directory to store object files **********

prepare:
	mkdir -p $(OBJECTDIRECTORY)


# ********** make dependency information among C source files **********

dep:
	@echo === Make Dependency File ===	
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===


# ********** move working directory to OBJECTDIRECTORY, make dependency file, elf file  **********

libMINT64OS: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile libMINT64OS.a
	cp $(OBJECTDIRECTORY)/libMINT64OS.a ./

# the file name MUST starts with "lib" and end with ".a"
# 
# in the process of combining object file with static-link library, 
# the Linker(LD) retrieves library by attatching "lib"/".a" ahead/rear of given library name


# ********** clean Temp dir **********

clean:
	rm -f *.a
	rm -f $(OBJECTDIRECTORY)/*.*



#####################################################
# re-called by Make, working dir is Temp
#####################################################

# ********** define C source files to build  **********

CSOURCEFILES 		= $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES		= $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))


# ********** define how to make .c file to .o file  **********

%.o: ../$(SOURCEDIRECTORY)/%.c
	 $(GCC64) -c $<


# ********** define how to make .asm file to .o file  **********

%.o: ../$(SOURCEDIRECTORY)/%.asm
	 $(NASM64) -o $@ $<


# ********** create dependency-related file **********

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep


# ********** include library used by Application Program as static-linked library **********

libMINT64OS.a: $(ASSEMBLYOBJECTFILES) $(COBJECTFILES)
			   $(AR64) $@ $^


# ********** if Dependency.dep exists, include it (to make) **********

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
