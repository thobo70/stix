cmake_minimum_required(VERSION 3.18)

set(CMAKE_ASM_COMPILER as)
enable_language(ASM)

project(
  os
  VERSION 0.0.1
  DESCRIPTION "a clone of an early unix"
  LANGUAGES C ASM
)

include(FetchContent)

FetchContent_Declare(
  cunit
  GIT_REPOSITORY https://gitlab.com/cunity/cunit.git
  GIT_TAG master
)

FetchContent_Declare(
  bochs
  GIT_REPOSITORY https://github.com/bochs-emu/Bochs.git
  GIT_TAG REL_3_0_FINAL
)

set(CUNIT_DISABLE_TESTS ON CACHE BOOL "Disable CUnit tests" FORCE)
set(CUNIT_DISABLE_EXAMPLES ON CACHE BOOL "Disable CUnit examples" FORCE)

FetchContent_MakeAvailable(cunit bochs)

# message("bochs_SOURCE_DIR: ${bochs_SOURCE_DIR}")

#begin: generate bochs, bochs doesn't have a cmake build system yet
set(bochs_CONFIGURE_PARAMS
  --prefix=${CMAKE_BINARY_DIR}    # install to build directory
  --enable-cpu-level=3            # enable cpu level 3
  --with-nogui                    # disable gui
  #--with-x11                      # enable x11
  --with-term                     # enable terminal
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/bin/bochs
  WORKING_DIRECTORY ${bochs_SOURCE_DIR}/bochs
  COMMAND ./configure ${bochs_CONFIGURE_PARAMS} > /dev/null
  COMMAND make all-clean > /dev/null
  COMMAND make > /dev/null
  COMMAND make install > /dev/null
  DEPENDS ${bochs_SOURCE_DIR}/bochs
)

add_custom_command(
  OUTPUT floppya.img
  COMMAND cp bootsec.bin floppya.img
  DEPENDS bootsec.bin
)

add_custom_command(
  OUTPUT bochsrc.txt
  COMMAND cp ${PROJECT_SOURCE_DIR}/tests/bochs/bochsrc.templ bochsrc.txt
  DEPENDS floppya.img ${PROJECT_SOURCE_DIR}/tests/bochs/bochsrc.templ
)

add_custom_target(
  generate-bochs-sim ALL
  DEPENDS ${CMAKE_BINARY_DIR}/bin/bochs bochsrc.txt
)
#end: generate bochs

set(SRC 
  src/buf/buf.c
  src/clist/clist.c
  src/fs/fs.c
  src/fs/inode.c
  src/fs/blocks.c
  src/dd/dd.c
  src/util/utils.c
  src/fsck/fsck.c
  src/mkfs/mkfs.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic-errors")

include(CTest)

# BEGIN: Assemble and link bootsec.s with bootsec.ld to generate bootsec.bin
add_custom_command(
  OUTPUT bootsec.bin
#  COMMAND ${CMAKE_ASM_COMPILER} -a -o bootsec.o -c "${PROJECT_SOURCE_DIR}/src/bootsec/bootsec.s"
  COMMAND as -a -o bootsec.o -c "${PROJECT_SOURCE_DIR}/src/bootsec/bootsec.s" > ./bootsec.lst
  COMMAND ${CMAKE_LINKER} -o bootsec.bin --oformat binary bootsec.o -T "${PROJECT_SOURCE_DIR}/src/bootsec/bootsec.ld"
  COMMAND echo "2nd sector loaded at 0x7e00" >> bootsec.bin
  COMMAND head -c 512 /dev/zero >> bootsec.bin
  BYPRODUCTS bootsec.o bootsec.lst bootsec.bin
  COMMENT "Assembling and linking bootsec.s to generate bootsec.bin"
  DEPENDS src/bootsec/bootsec.s src/bootsec/bootsec.ld
)

add_custom_target(
  bootsec ALL
  DEPENDS bootsec.bin
)
# END: Assemble and link bootsec.s with bootsec.ld to generate bootsec.bin

add_executable(
  mockup
  src/main.c
  src/pc/pc.c
  ${SRC}
)

target_include_directories(
  mockup
  PUBLIC src/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Note: Console driver demonstration removed - requires kernel/hardware access

# Modular test suite configuration
set(TEST_COMMON_SRC
  tests/common/test_common.c
  tests/tstcon.c
  tests/tstdisk.c
  tests/pc.mockup.c
)

set(TEST_UNIT_SRC
  tests/unit/test_system.c
  tests/unit/test_buffer.c
  tests/unit/test_clist.c
  tests/unit/test_filesystem.c
  tests/unit/test_inode.c
  tests/unit/test_clist_tstcon.c
  tests/unit/test_fsck.c
  tests/unit/test_mkfs.c
  tests/unit/test_mount.c
  tests/unit/test_endian.c
  tests/unit/test_active_inode_detection.c
)

# Build modular test executable
add_executable(
  tests_modular
  tests/test_main.c
  ${TEST_COMMON_SRC}
  ${TEST_UNIT_SRC}
  ${SRC}
)

target_include_directories(
  tests_modular
  PUBLIC src/include
  PUBLIC tests
)

target_link_libraries(tests_modular PRIVATE cunit)

# Keep legacy tests1 for compatibility
add_executable(
  tests1
  tests/tests1.c
  tests/tstcon.c
  tests/tstdisk.c
  tests/pc.mockup.c
  ${SRC}
)

target_include_directories(
  tests1
  PUBLIC src/include
)

target_link_libraries(tests1 PRIVATE cunit)

# Standalone filesystem check utility
add_executable(
  fsck
  src/fsck/fsck_main.c
  src/fsck/fsck.c
)

target_include_directories(
  fsck
  PUBLIC src/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Standalone make filesystem utility
add_executable(
  mkfs
  src/mkfs/mkfs_main.c
  src/mkfs/mkfs.c
)

target_include_directories(
  mkfs
  PUBLIC src/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME mockup_test
  COMMAND mockup
)

add_test(
  NAME tests_modular
  COMMAND tests_modular
)
set_property(TEST tests_modular PROPERTY TIMEOUT "15")

add_test(
  NAME tests1
  COMMAND tests1
)
set_property(TEST tests1 PROPERTY TIMEOUT "10")

add_test(
  NAME bochs_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND sh -c "bin/bochs -q -f bochsrc.txt ; exit 0"
)
set_tests_properties(bochs_test PROPERTIES
  PASS_REGULAR_EXPRESSION "shutdown\ requested"
)
set_property(TEST bochs_test PROPERTY TIMEOUT "10")

find_package(Java 8 COMPONENTS Runtime)

file(
  DOWNLOAD 
  https://github.com/plantuml/plantuml/releases/latest/download/plantuml.jar.asc 
  ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc
  TIMEOUT 10
  STATUS DOWNLOAD_STATUS
)
list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
message("downloading plantuml.jar.asc: ${ERROR_MESSAGE}")
if (STATUS_CODE EQUAL 0)
  file(READ ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc PLANTUML_NEW)
  file(TOUCH ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc.old)
  file(READ ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc.old PLANTUML_OLD)
  if (NOT PLANTUML_NEW STREQUAL PLANTUML_OLD)
    file(
      DOWNLOAD
      https://github.com/plantuml/plantuml/releases/latest/download/plantuml.jar
      ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar
      TIMEOUT 10
      SHOW_PROGRESS
    )  
    file(RENAME ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc ${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar.asc.old)
  endif()
endif()

find_package(
  Doxygen
  REQUIRED doxygen dot
  OPTIONAL_COMPONENTS mscgen dia
)

set(DOXYGEN_EXCLUDE_PATTERNS  */build/*)
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_MAN YES)
set(DOXYGEN_GENERATE_LATEX YES)
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_CALLER_GRAPH YES)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_PLANTUML_JAR_PATH "${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar")
set(DOXYGEN_IMAGE_PATH "${PROJECT_SOURCE_DIR}")
set(DOXYGEN_WARN_LOGFILE "DoxygenWarningLog.txt")

doxygen_add_docs(
  docs
)
