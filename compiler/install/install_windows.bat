@echo off
setlocal EnableDelayedExpansion

set "INSTALL_DIR=%USERPROFILE%\.local\bin"
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

call :check_dependencies
if !errorlevel! neq 0 (

    echo Not all the required libraries for the build are installed on your OS. Required libraries: cmake, ninja-build, clang-21, llvm-21-dev, bison, flex, libspdlog-dev.
    echo Let's try building it using Docker.

    call :install_with_docker

    if !errorlevel! equ 0 (
        echo Compiler paracl successfully installed to %INSTALL_DIR%
        exit /b 0
    ) else (
        echo Installation has failed.
        exit /b 1
    )
) else (
    call :install_without_docker

    if !errorlevel! equ 0 (
        echo Compiler paracl successfully installed to %INSTALL_DIR%
        exit /b 0
    ) else (
        echo Installation has failed.
        exit /b 1
    )
)


:check_dependencies
    where cmake >nul 2>nul || exit /b 1
    where ninja >nul 2>nul || exit /b 1
    where clang++ >nul 2>nul || exit /b 1
    where bison >nul 2>nul || exit /b 1
    where flex >nul 2>nul || exit /b 1
    exit /b 0


:install_with_docker
    where docker >nul 2>nul

    if !errorlevel! neq 0 (
        echo Docker is not installed on your system. The installer requires it.
        exit /b 1
    )

    docker build -t paracl compiler\.

    (
    echo @echo off
    echo docker run -it -v "%%cd%%:/cwd" -w /cwd paracl %%*
    ) > "%INSTALL_DIR%\paracl.bat"
    exit /b !errorlevel!


:install_without_docker
    cmake -G Ninja -S compiler/ -B compiler/build -DCMAKE_CXX_COMPILER=clang++
    if !errorlevel! neq 0 exit /b 1

    cmake --build compiler/build
    if !errorlevel! neq 0 exit /b 1

    copy /Y "compiler\build\paracl" "%INSTALL_DIR%\"
    exit /b !errorlevel!
