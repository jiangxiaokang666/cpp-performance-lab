cmake_minimum_required(VERSION 3.16)
project(rgba_to_bgra_benchmark LANGUAGES CXX)

add_executable(rgba_to_bgra
    src/main.cpp
)

target_compile_features(rgba_to_bgra PRIVATE cxx_std_17)

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|X86_64)$")
    target_sources(rgba_to_bgra PRIVATE
        src/rgba_to_bgra_ssse3.cpp
        src/rgba_to_bgra_avx2.cpp
        src/rgba_to_bgra_avx2_nt.cpp)
    target_compile_definitions(rgba_to_bgra PRIVATE
        HAVE_SSSE3_IMPLEMENTATION=1
        HAVE_AVX2_IMPLEMENTATION=1)

    if(MSVC)
        set_source_files_properties(
            src/rgba_to_bgra_avx2.cpp
            src/rgba_to_bgra_avx2_nt.cpp
            PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
        target_compile_options(rgba_to_bgra PRIVATE /W4)
    else()
        set_source_files_properties(src/rgba_to_bgra_ssse3.cpp
            PROPERTIES COMPILE_OPTIONS "-mssse3")
        set_source_files_properties(
            src/rgba_to_bgra_avx2.cpp
            src/rgba_to_bgra_avx2_nt.cpp
            PROPERTIES COMPILE_OPTIONS "-mavx2")
        target_compile_options(rgba_to_bgra PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
