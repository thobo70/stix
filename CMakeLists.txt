cmake_minimum_required(VERSION 3.18)

project(
  os
  VERSION 0.1.1
  DESCRIPTION "a clone of an early unix"
  LANGUAGES C 
)

set(SRC 
  src/main.c
  src/pc/pc.c
  src/buf/buf.c
  src/fs/fs.c
  src/fs/inode.c
  src/fs/blocks.c
  src/dd/dd.c
  src/dd/tstcon.c
  src/dd/tstdisk.c
  src/util/utils.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic-errors")

add_subdirectory(cunit/CUnit)

find_package(Java 8 COMPONENTS Runtime)

include(CTest)

find_package(
  Doxygen
  REQUIRED doxygen dot
  OPTIONAL_COMPONENTS mscgen dia
)

add_executable(
  mockup
  ${SRC}
)

set_target_properties( mockup PROPERTIES COMPILE_FLAGS "-Wextra" )

target_include_directories(
  mockup
  PUBLIC src/include
)

add_executable(
  tests1
  ${SRC}
  tests/tests1.c
)

set_target_properties( tests1 PROPERTIES COMPILE_FLAGS "-Wextra" )

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

set(DOXYGEN_EXCLUDE_PATTERNS */cunit/*)
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
