#project config
cmake_minimum_required(VERSION 3.10)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

SET(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_COMPILER x86_64-elf-g++)
set(CMAKE_C_COMPILER x86_64-elf-gcc)

# cheat the compile test
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_CROSSCOMPILING 1)

include(FetchContent)
set(FETCHCONTENT_QUIET false)

project(chroma)
enable_language(ASM-NASM)
enable_language(C)
enable_language(CXX)

SET(src_files
        ${CMAKE_SOURCE_DIR}/src/kernel.cpp
        ${CMAKE_SOURCE_DIR}/src/video/draw.cpp
        ${CMAKE_SOURCE_DIR}/src/video/print.cpp
        ${CMAKE_SOURCE_DIR}/src/system/cpu.cpp
        ${CMAKE_SOURCE_DIR}/src/system/core.cpp
        ${CMAKE_SOURCE_DIR}/src/global/core.s
        ${CMAKE_SOURCE_DIR}/src/system/rw.cpp
        ${CMAKE_SOURCE_DIR}/src/system/serial.cpp
        ${CMAKE_SOURCE_DIR}/src/system/pci.cpp
        ${CMAKE_SOURCE_DIR}/src/system/acpi/MADT.cpp
        ${CMAKE_SOURCE_DIR}/src/system/acpi/RSDP.cpp
        ${CMAKE_SOURCE_DIR}/src/system/memory/paging.cpp
        ${CMAKE_SOURCE_DIR}/src/system/memory/abstract_allocator.cpp
        ${CMAKE_SOURCE_DIR}/src/system/memory/liballoc.cpp
        ${CMAKE_SOURCE_DIR}/src/system/memory/physmem.cpp
        ${CMAKE_SOURCE_DIR}/src/system/process/process.cpp
        ${CMAKE_SOURCE_DIR}/src/drivers/elf.cpp
        ${CMAKE_SOURCE_DIR}/src/drivers/devices/devices.cpp
        ${CMAKE_SOURCE_DIR}/src/drivers/devices/input/keyboard.cpp
        ${CMAKE_SOURCE_DIR}/src/drivers/devices/io/apic.cpp
        ${CMAKE_SOURCE_DIR}/src/drivers/devices/storage/ata.cpp
)

SET(lib_files
        ${CMAKE_SOURCE_DIR}/src/lainlib/list/basic_list.cpp
        ${CMAKE_SOURCE_DIR}/src/lainlib/mutex/ticketlock.cpp
        ${CMAKE_SOURCE_DIR}/src/lainlib/compression/lzgmini.c
        ${CMAKE_SOURCE_DIR}/src/lainlib/string/str.cpp
        ${CMAKE_SOURCE_DIR}/src/editor/EditorMain.cpp
)

include_directories("inc" "D:/mingw/mingw64/lib/gcc/x86_64-w64-mingw32/8.1.0/include/c++" "D:/mingw/mingw64/lib/gcc/x86_64-w64-mingw32/8.1.0/include/c++/x86_64-w64-mingw32")

SET(src_no_sse
        ${CMAKE_SOURCE_DIR}/src/system/interrupts.cpp
)

SET(src_preamble
        ${CMAKE_SOURCE_DIR}/src/global/crt0.o
        ${CMAKE_SOURCE_DIR}/src/global/crti.o
        ${CMAKE_SOURCE_DIR}/src/global/crtbegin.o
)

set(src_epilogue
        ${CMAKE_SOURCE_DIR}/src/global/crtend.o
        ${CMAKE_SOURCE_DIR}/src/global/crtn.o
        ${CMAKE_SOURCE_DIR}/src/assets/font.o
        ${CMAKE_SOURCE_DIR}/src/assets/zerosharp.o
)

set_property(SOURCE ${src_no_sse} PROPERTY COMPILE_FLAGS -mgeneral-regs-only)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(kernel)

target_sources(kernel PUBLIC ${src_preamble} PUBLIC ${src_files} PUBLIC ${src_no_sse} PUBLIC ${lib_files} PUBLIC ${src_epilogue})
target_compile_options(kernel PRIVATE -ffreestanding -O0 -Wall -Wextra -Wall -Werror -fPIC -fno-exceptions -fno-omit-frame-pointer -mno-red-zone -fno-stack-protector $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> -ggdb3)
target_link_options(kernel PRIVATE -T ${CMAKE_SOURCE_DIR}/linker.ld -ffreestanding -O2 -nostdlib -nostartfiles -lgcc)

add_custom_command(
        COMMENT "Generating src/assets/font.o"
        OUTPUT "${CMAKE_SOURCE_DIR}/src/assets/font.o"
        DEPENDS "${CMAKE_SOURCE_DIR}/src/assets/font.psf"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND x86_64-elf-ld -r -b binary -o "src/assets/font.o" "src/assets/font.psf"
)

set(_bootboot "${CMAKE_BINARY_DIR}/_deps/bootboot")
FetchContent_Declare(
        bootboot
        SOURCE_DIR "${_bootboot}"
        GIT_PROGRESS true
        GIT_REPOSITORY "https://gitlab.com/bztsrc/bootboot.git"
        GIT_TAG "master"
        GIT_SHALLOW true
)
FetchContent_MakeAvailable(bootboot)
set(_mkbootimg_makefile "${_bootboot}/mkbootimg/Makefile")
file(READ "${_mkbootimg_makefile}" _mkbootimg_makefile_text)
string(REGEX REPLACE "@zip" "#@zip" _mkbootimg_makefile_text "${_mkbootimg_makefile_text}")
file(WRITE "${_mkbootimg_makefile}" "${_mkbootimg_makefile_text}")

set(_mkbootimg "${CMAKE_BINARY_DIR}/bin/mkbootimg${CMAKE_EXECUTABLE_SUFFIX}")
add_custom_command(
        COMMENT "Generating mkbootimg${CMAKE_EXECUTABLE_SUFFIX}"
        OUTPUT "${_mkbootimg}"
        WORKING_DIRECTORY "${_bootboot}/mkbootimg"
        COMMAND make all
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "mkbootimg${CMAKE_EXECUTABLE_SUFFIX}" "${_mkbootimg}"
)
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/img/loader")
add_custom_command(
        COMMENT "Generating img/loader/bootloader"
        OUTPUT "${CMAKE_SOURCE_DIR}/img/loader/bootloader"
        DEPENDS "${CMAKE_SOURCE_DIR}/bin/kernel"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bootboot}/dist/boot.bin" "${CMAKE_SOURCE_DIR}/img/loader/bootloader"
)
add_custom_command(
        COMMENT "Generating img/boot/exe"
        OUTPUT "${CMAKE_SOURCE_DIR}/img/boot/exe"
        DEPENDS "${CMAKE_SOURCE_DIR}/bin/kernel"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_SOURCE_DIR}/bin/kernel" "${CMAKE_SOURCE_DIR}/img/boot/exe"
)
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/img")
add_custom_command(
        COMMENT "Generating bin/img/chroma.img"
        OUTPUT "${CMAKE_SOURCE_DIR}/bin/img/chroma.img"
        DEPENDS "${_mkbootimg}" "${CMAKE_SOURCE_DIR}/img/loader/bootloader" "${CMAKE_SOURCE_DIR}/img/boot/exe"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND "${_mkbootimg}" "${CMAKE_SOURCE_DIR}/chroma.json" "${CMAKE_SOURCE_DIR}/bin/img/chroma.img"
)
add_custom_target(chroma.img DEPENDS "${CMAKE_SOURCE_DIR}/bin/img/chroma.img")
