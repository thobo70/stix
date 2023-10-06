# A small UNIX OS
## Aim of the project
An Unix like system for small 8 and 16-Bit Computers, which should spend as little as possible ram during execution. In addition, the whole suite should be able build itself on the small Computer.

## Prepare for developping
for doxygen support following packages needs to be installed:
```
sudo apt update
sudo apt upgrade -y
sudo apt install doxygen doxygen-latex doxygen-doc doxygen-gui graphviz xapian-tools
```

## CUnit
cunit is integrated as a git submodule and must be initialized for first use
```
cd cunit
git submodule init
git submodule update
```

## Sources of documents
for inspiration you can read following documents. Especially the first link covers the aimed functionality best
 * https://books.google.de/books/about/The_Design_of_the_UNIX_Operating_System.html?id=NrBQAAAAMAAJ&redir_esc=y
 * https://warsus.github.io/lions-/




