cmake_minimum_required(VERSION 3.18)

project(
  os
  VERSION 0.1.1
  DESCRIPTION "a clone of an early unix"
  LANGUAGES C
)

include(CTest)

find_package(
  Doxygen
  REQUIRED doxygen dot
  OPTIONAL_COMPONENTS mscgen dia
)

add_executable(
  mockup
  src/main.c
  src/pc/pc.c
  src/buf/buf.c
  src/fs/fs.c
  src/fs/inode.c
  src/fs/blocks.c
)

target_include_directories(
  mockup
  PUBLIC src/include
)

add_test(
  NAME mockup_test
  COMMAND mockup
)

set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_MAN YES)
set(DOXYGEN_GENERATE_LATEX YES)
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_CALLER_GRAPH YES)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_PLANTUML_JAR_PATH "${PROJECT_SOURCE_DIR}/plantuml/plantuml.jar")

doxygen_add_docs(
  docs
)
