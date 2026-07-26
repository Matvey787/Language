# ParaCL

```txt
ParaCL is a custom programming language compiler developed as an educational project during
the second year of MIPT university. The project was proposed by Konstantin Vladimirovich as
part of the YADRO course curriculum.
```

## Installation

```bash
git clone https://github.com/Matvey787/Language.git && cd Language
```

You'll need `make` and `docker` to install it.

```bash
make --makefile=compiler/Makefile install
```

Or without `make`:

```bash
# Linux
chmod +x compiler/install/install_unix.sh
compiler/install/install_unix.sh

# Windows
.\compiler\install\install_windows.bat
```

> [!NOTE]
> The installer will not use docker if the required libraries are installed: `cmake`, `ninja-build`, `clang-21`, `llvm-21-dev`, `bison`, `flex`, `libspdlog-dev`. (Linux only)

## Usage

```bash
paracl --help
```

## Additionally

For contributors, or to find out how everything works behind the scenes, see: [ABOUT.md](ABOUT.md)
