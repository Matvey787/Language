ifeq ($(OS),Windows_NT)
    DETECTED_OS := windows
else
    UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		DETECTED_OS := linux
	else
		$(error Unfortunately, this operating system $(UNAME_S) is not supported.)
	endif
endif

.PHONY: install install_linux install_windows

install: install_$(DETECTED_OS)

install_linux:
	chmod +x compiler/install/install_unix.sh
	compiler/install/install_unix.sh

install_windows:
	.\compiler\install\install_windows.bat