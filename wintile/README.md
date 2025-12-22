# WinVimTiler Build Instructions

This document provides the steps to build the WinVimTiler project using CMake.

## Prerequisites

- CMake 3.10 or higher
- A C++ compiler that supports C++20 (e.g., MSVC, GCC, Clang)

## Build Steps

1.  **Create a build directory:**
    It is recommended to keep build files out of the source directory.

    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    ```

2.  **Build the project:**
    Use CMake to build the project. This will compile the source code and create the executable.

    ```bash
    cmake --build build --config Release
    ```

3.  **Run the application:**
    The executable `WinVimTiler.exe` will be located in the `Release` directory.

    ```bash
    Release/WinVimTiler.exe
    ```
