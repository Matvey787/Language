@echo off
setlocal EnableDelayedExpansion

set "INSTALL_DIR=%USERPROFILE%\.local\bin"
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo The auto-installer for Windows only works through Docker.

call :install_with_docker

if !errorlevel! equ 0 (
    echo Compiler paracl successfully installed to %INSTALL_DIR%
    exit /b 0
) else (
    echo Installation has failed.
    exit /b 1
)

goto :EOF

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
    ) > "%INSTALL_DIR%\paracl"

    powershell -NoProfile -ExecutionPolicy Bypass -Command "$dir='%INSTALL_DIR%'; $p=[Environment]::GetEnvironmentVariable('Path', 'User'); if ($p -notlike ('*' + $dir + '*')) { [Environment]::SetEnvironmentVariable('Path', $p + ';' + $dir, 'User') }"

    echo Please, reopen command line (power shell).

    exit /b !errorlevel!

