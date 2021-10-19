# directories to build
all : MINT64OSLibrary HelloWorld TextViewer Shooter Hexa HangulViewer


# execute make at UserLibrary dir to build library
MINT64OSLibrary:
	@echo
	@echo =============== Build UserLibrary ===============
	@echo

	make -C UserLibrary

	@echo
	@echo =============== Build Complete ===============
	@echo


# execute make at HelloWorld dir to build Hello World Application Program
HelloWorld:
	@echo
	@echo =============== Build HelloWorld ===============
	@echo

	make -C 00.HelloWorld

	@echo
	@echo =============== Build Complete ===============
	@echo


# execute make at TextViewer dir to build Text Viewer
TextViewer:
	@echo
	@echo =============== Build TextViewer ===============
	@echo

	make -C 01.TextViewer

	@echo
	@echo =============== Build Complete ===============
	@echo


# execute make at Shooter dir to build Bubble Shooter Game
Shooter:
	@echo
	@echo =============== Build Shooter ===============
	@echo

	make -C 02.Shooter

	@echo
	@echo =============== Build Complete ===============
	@echo


# execute make at Shooter dir to build Hexa Game
Hexa:
	@echo
	@echo =============== Build Hexa ===============
	@echo

	make -C 03.Hexa

	@echo
	@echo =============== Build Complete ===============
	@echo


# execute make at HangulViewer dir to build Hangul Viewer
HangulViewer:
	@echo
	@echo =============== Build HangulViewer ===============
	@echo

	make -C 04.HangulViewer

	@echo
	@echo =============== Build Complete ===============
	@echo



# clean files
clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
	make -C 01.TextViewer clean
	make -C 02.Shooter clean
	make -C 03.Hexa clean
	make -C 04.HangulViewer clean
