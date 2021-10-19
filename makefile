all: BootLoader Kernel32 Kernel64 Disk.img Application Utility


# exec makefiles
BootLoader:
	@echo 
	@echo ========== Build Boot Loader ==========
	@echo 

	make -C 00.BootLoader

	@echo 
	@echo ========== Build Complete ==========
	@echo 
	
Kernel32:
	@echo
	@echo ========== Build 32-bit Kernel ==========
	@echo

	make -C 01.Kernel32

	@echo
	@echo ========== Build Complete ==========
	@echo


Kernel64:
	@echo
	@echo ========== Build 64-bit Kernel ==========
	@echo

	make -C 02.Kernel64

	@echo
	@echo ========== Build Complete ==========
	@echo


# copy build result to OS Image
Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin
	@echo 
	@echo ========== Disk Image Build Start ==========
	@echo 

	./ImageMaker.exe $^
	cat Disk.img Package.img > DiskWithPackage.img

	@echo 
	@echo ========== All Build Complete ==========
	@echo 


# build Application Program
Application:
	@echo 
	@echo ========== Application Build Start ==========
	@echo 

#	make -C 03.Application

	@echo 
	@echo ========== Application Build Complete ==========
	@echo 


Utility:
	@echo
	@echo ========== Utility Build Start ==========
	@echo

	make -C 04.Utility

	@echo
	@echo ========== Utility Build Complete ==========
	@echo


clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 03.Application clean
	make -C 04.Utility clean
	rm -f Disk.img
	rm -f DiskWithPackage.img
