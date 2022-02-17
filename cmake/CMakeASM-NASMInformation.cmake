SET(ASM_DIALECT "-NASM")

SET(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS s;nasm;nas;asm)

if(WIN32)
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT win64)
    else()
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT win32)
    endif()
elseif(APPLE)
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT macho64)
    else()
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT macho)
    endif()
else()
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT elf64)
    else()
        SET(CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT elf)
    endif()
endif()

# This section exists to override the one in CMakeASMInformation.cmake
# (the default Information file). This removes the <FLAGS>
# thing so that your C compiler flags that have been set via
# set_target_properties don't get passed to nasm and confuse it.
IF (NOT CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT)
    SET(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> -f ${CMAKE_ASM${ASM_DIALECT}_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")
ENDIF (NOT CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT)

INCLUDE(CMakeASMInformation)

SET(ASM_DIALECT)
