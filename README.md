# `ninobc`

A simple bootstrap compiler for a subset of the entire Nino programming language written in C.
Almost no error handling or type checking is done. Emits equivalent C code.

To build the compiler from source using an installed C compiler (`cc`),
simply run the `build.sh`-script at the repository root.

Usage:
```
ninobc <main-function> <files>
```
where `<files>` is any number of paths of `.nino` source files
and `<main-function>` is the full path of a function without any arguments.