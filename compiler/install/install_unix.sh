#!/bin/bash

INSTALL_DIR="$HOME/.local/bin"  

mkdir -p "$INSTALL_DIR"


install_with_docker()
{
   
if ! command -v docker > /dev/null 2>&1; then
    echo "Docker is not installed on your system. The installer requires it."
    exit 1
fi

docker build -t paracl compiler/.

cat > "$INSTALL_DIR/paracl" << 'DOCKER_WRAPPER'
#!/bin/bash
docker run -it -v $(pwd):/cwd -w /cwd paracl "$@"
DOCKER_WRAPPER

chmod +x "$INSTALL_DIR/paracl"

if [ $? -eq 0 ]; then
    echo "Compiler paracl successfully installed to $INSTALL_DIR"
else
    echo "Instalation has failed."
fi

}

install_without_docker()
{
    cmake -G=Ninja -S compiler/ -B compiler/build -DCMAKE_CXX_COMPILER=clang++ || return 1

    cmake --build compiler/build || return 1

    cp compiler/build/paracl "$INSTALL_DIR/" || return 1

    return 0
}

check_dependencies()
{
      command -v cmake                                       >/dev/null 2>&1 || { echo "cmake does not exist."; return 1; }
      command -v bison                                       >/dev/null 2>&1 || { echo "bison does not exist."; return 1; }
      command -v flex                                        >/dev/null 2>&1 || { echo "flex does not exist." ; return 1; }
    { command -v ninja          || command -v ninja-build; } >/dev/null 2>&1 || { echo "ninja does not exist."; return 1; }
    { command -v clang-21       || command -v clang;       } >/dev/null 2>&1 || { echo "clang does not exist."; return 1; }
    { command -v llvm-config-21 || command -v llvm-config; } >/dev/null 2>&1 || { echo "llvm does not exist." ; return 1; }
    pkg-config --exists spdlog >/dev/null 2>&1 || return 1

    return 0
}

if ! check_dependencies; then
    echo -e "Not all the required libraries for the build are installed on your OS. Required libraries: cmake, ninja-build, clang-21, llvm-21-dev, bison, flex, libspdlog-dev."
    echo "Let's try building it using Docker."

    install_with_docker
else
    install_without_docker
fi
