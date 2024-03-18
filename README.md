# `ninobc`

A simple bootstrap compiler for a subset of the entire Nino programming language written in C.
Almost no error handling or type checking is done. Emits equivalent C code.

To build the compiler from source simply use `make` (`make CC=my_compiler` to use `my_compiler`).

Usage:
```
ninobc <args> <files>
```
where `<files>` is any number of input file paths and `<args>` may be:
- `-m <main>` - specifies the full path of the main function
- `-o <path>` - specifies the output file name 
