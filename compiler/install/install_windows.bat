@echo off
setlocal EnableDelayedExpansion

set "INSTALL_DIR=%USERPROFILE%\.local\bin"
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo The auto-installer for Windows only works through Docker.

call :install_with_docker

if !errorlevel! equ 0 (
        echo "Compiler paracl successfully installed to $INSTALL_DIR"
cat << 'BANNER' 
 ____                      ____  _     
|  _ \  __ _  _ __  __ _  / ___|| |    
| |_) |/ _` || '__|/ _` || |    | |    
|  __/| (_| || |  | (_| || |___ | |___ 
|_|    \__,_||_|   \__,_| \____||_____|
BANNER

    echo "Compiler paracl successfully installed to $INSTALL_DIR"
    echo -e "For questions, suggestions, or contributions, feel free to contact: matveyklg@gmail.com. If you encounter any bugs, issues, or have interesting \nfindings, please open an issue or make pull request at: https://github.com/Matvey787/Language." | fold -s -w 80
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

