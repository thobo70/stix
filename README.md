# A small UNIX OS

![stix (super tiny IX)](stix_logo.png)

## Aim of the project "Super Tiny unIX" (stix)
An Unix like system for small 8 and 16-Bit Computers, which should spend as little as possible ram during execution. In addition, the whole suite (future) should be able build itself on the small Computer.

## Getting started
for developping following packages needs to be installed:
```
sudo apt install build-essential llvm clang gdb git git-lfs cmake default-jre
sudo apt install doxygen doxygen-latex doxygen-doc doxygen-gui graphviz xapian-tools
git lfs install
git config --global user.name "your name"
git config --global user.email your@email.com
git config --global http.sslVerify false
```

## CUnit
cunit is integrated as a git submodule and must be initialized for first use
```
cd cunit
git submodule init
git submodule update
```
 * https://cunit.sourceforge.net/doc/index.html
 * https://cunit.sourceforge.net/doxdocs/index.html


## Sources of documents
for inspiration you can read following documents. Especially the first link covers the aimed functionality best
 * https://books.google.de/books/about/The_Design_of_the_UNIX_Operating_System.html?id=NrBQAAAAMAAJ&redir_esc=y
 * https://warsus.github.io/lions-/
 * https://plantuml.com/en/sitemap
 * https://cunit.sourceforge.net/documentation.html
 




