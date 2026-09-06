FROM ubuntu:latest AS build

RUN apt-get update && apt-get install -y cmake ninja-build clang-21 llvm-21-dev bison flex libspdlog-dev

RUN ln -s /usr/bin/clang++-21 /usr/bin/clang
RUN ln -s /usr/bin/clang++-21 /usr/bin/clang++

WORKDIR /compiler

COPY . .

RUN cmake -G=Ninja -S . -B build -DCMAKE_CXX_COMPILER=clang++

RUN cmake --build build --parallel

ENTRYPOINT ["/compiler/build/paracl"]
