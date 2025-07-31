cmake_minimum_required(VERSION 3.10)

project(WinVimTiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directories for executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Enable modern Windows API features
add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    VC_EXTRALEAN
    STRICT
    UNICODE
    _UNICODE
    NOMINMAX
)

add_executable(WinVimTiler WIN32 main.cpp)

# High-Performance Compiler Optimizations
if(MSVC)
    # Target modern Windows versions and enable specific optimizations
    target_compile_definitions(WinVimTiler PRIVATE _WIN32_WINNT=0x0A00)

    # Use modern CMake for compiler options
    target_compile_options(WinVimTiler PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Ot /GT /GL /arch:AVX2 /fp:fast /GS- /Gy>
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    )

    set_target_properties(WinVimTiler PROPERTIES
        LINK_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF"
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang optimizations
    target_compile_options(WinVimTiler PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endif()

# Link against necessary Windows libraries
target_link_libraries(WinVimTiler PRIVATE user32 gdi32 dwmapi)

# Set high DPI awareness for better performance on modern displays
if(MSVC)
    set_target_properties(WinVimTiler PROPERTIES
        VS_DPI_AWARE "PerMonitor"
        WIN32_EXECUTABLE TRUE
    )
endif()

# Enable Link Time Optimization (LTO/IPO) for Release builds
set_property(TARGET WinVimTiler PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
