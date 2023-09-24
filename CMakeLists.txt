cmake_minimum_required(VERSION 3.18)

project(
  os
  VERSION 0.1.1
  DESCRIPTION "a clone of early unix"
  LANGUAGES C
)

find_package(
  Doxygen
  REQUIRED doxygen dot
  OPTIONAL_COMPONENTS mscgen dia
)

add_executable(
  os
  src/pc/pc.c
  src/buf/buf.c
  src/fs/fs.c
  src/fs/inode.c
#   error.c
#   input.c
#   lexer.c
#   macro.c
#   main.c
#   parser.c
#   rule.c
)

target_include_directories(
  os
  PUBLIC src/include
)

set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_MAN YES)
set(DOXYGEN_GENERATE_LATEX YES)
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_CALLER_GRAPH YES)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)

doxygen_add_docs(
  docs
  $(CMAKE_CURRENT_SOURCE_DIR)
)
