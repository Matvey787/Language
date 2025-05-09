# Qt Application

This is a Qt-based application that provides a graphical user interface.

## Prerequisites

- CMake (version 3.16 or higher)
- Qt6 with Widgets module
- C++17 compatible compiler
- Make or Ninja build system

## Building the Project

1. Create a build directory:

```bash
mkdir build
cd build
```

2. Configure the project with CMake:

```bash
cmake ..
```

3. Build the project:

```bash
make
```

4. Run the application:

```bash
./qt_app
```

## Project Structure

- `main.cpp` - Main application entry point
- `mainwindow.ui` - Qt Designer UI file
- `ui_mainwindow.h` - Auto-generated UI header file
- `CMakeLists.txt` - CMake build configuration

## Development

The project uses CMake as its build system and is configured to:
- Use C++17 standard
- Automatically process Qt MOC, RCC, and UIC files
- Link against Qt6 Widgets module

## Troubleshooting

If you encounter any build issues:

1. Make sure Qt6 is properly installed and available in your system
2. Verify that CMake can find Qt6 by checking the CMake output
3. Ensure you have the required build tools installed
4. Check that you're running the commands from the correct directory

## License

[Add your license information here] 