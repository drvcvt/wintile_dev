# WinVimTiler Build Instructions

This document provides the steps to build the WinVimTiler project using CMake.

## Prerequisites

- CMake 3.10 or higher
- A C++ compiler that supports C++20 (e.g., MSVC, GCC, Clang)

## Build Steps

1.  **Create a build directory:**
    It is recommended to create a separate directory for the build files to keep the source directory clean.

    ```bash
    mkdir build
    cd build
    ```

2.  **Run CMake to generate the build system:**
    From within the `build` directory, run CMake to configure the project.

    ```bash
    cmake ..
    ```

3.  **Build the project:**
    Use CMake to build the project. This will compile the source code and create the executable.

    ```bash
    cmake --build . --config Release
    ```

4.  **Run the application:**
    The executable `WinVimTiler.exe` will be located in the `wintile/bin` directory.

    ```bash
    ../wintile/bin/WinVimTiler.exe
    ```
