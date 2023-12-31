cmake_minimum_required(VERSION 3.18)

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
  GIT_TAG 9cf44aa3a37cfa5570dd70e122b09bfa6e711fee
)

set(CUNIT_DISABLE_TESTS ON CACHE BOOL "Disable CUnit tests" FORCE)
set(CUNIT_DISABLE_EXAMPLES ON CACHE BOOL "Disable CUnit examples" FORCE)

FetchContent_MakeAvailable(cunit)

set(SRC 
  src/buf/buf.c
  src/clist/clist.c
  src/fs/fs.c
  src/fs/inode.c
  src/fs/blocks.c
  src/dd/dd.c
  src/util/utils.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic-errors")

include(CTest)

# BEGIN: Assemble and link bootsec.s with bootsec.ld to generate bootsec.bin
add_custom_command(
  OUTPUT bootsec.bin
  COMMAND ${CMAKE_ASM_COMPILER} -o bootsec.o -c "${PROJECT_SOURCE_DIR}/src/bootsec/bootsec.s"
  COMMAND ${CMAKE_LINKER} -o bootsec.bin --oformat binary bootsec.o -T "${PROJECT_SOURCE_DIR}/src/bootsec/bootsec.ld"
  BYPRODUCTS bootsec.o
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
)

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

add_test(
  NAME mockup_test
  COMMAND mockup
)

add_test(
  NAME tests1
  COMMAND tests1
)
set_property(TEST tests1 PROPERTY TIMEOUT "10")

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
