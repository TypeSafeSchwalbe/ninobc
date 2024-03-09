# `ninoi`

A simple bootstrap interpreter for a subset of the entire Nino programming language, used for self-hosting, written in C. Almost no error handling or type checking is done.

To build the interpreter from source using an installed C compiler (`cc`),
simply run the `build.sh`-script at the repository root.

Usage:
```
ninoi <main-function> <files...>
```