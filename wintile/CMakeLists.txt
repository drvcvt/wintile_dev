cmake_minimum_required(VERSION 3.10)

project(WinVimTiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable modern Windows API features for better performance
add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    VC_EXTRALEAN
    STRICT
    UNICODE
    _UNICODE
)

add_executable(WinVimTiler WIN32 main.cpp)

# High-Performance Compiler Optimizations
if(MSVC)
    # Microsoft Visual C++ optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Ot /GT /GL /arch:AVX2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast /GS- /Gy /GF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    
    # Enable SIMD intrinsics and modern CPU features
    target_compile_definitions(WinVimTiler PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)
    
    # Enable branch prediction and modern optimizations
    target_compile_options(WinVimTiler PRIVATE 
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG /arch:AVX2 /fp:fast>
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -ffast-math -funroll-loops")
    
    target_compile_options(WinVimTiler PRIVATE 
        $<$<CONFIG:Release>:-O3 -march=native -flto -DNDEBUG>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endif()

# Link against necessary Windows libraries
target_link_libraries(WinVimTiler PRIVATE user32 gdi32)

# Set high DPI awareness for better performance on modern displays
if(MSVC)
    set_target_properties(WinVimTiler PROPERTIES
        VS_DPI_AWARE "PerMonitor"
        WIN32_EXECUTABLE TRUE
    )
endif()

# Enable Link Time Optimization for maximum performance
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET WinVimTiler PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif() 