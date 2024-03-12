
#include <stdio.h>
#include "parser.h"
#include "symbols.h"

static String read_file(const char* path, Arena* arena) {
    FILE* f = fopen(path, "r");
    if(f == NULL) { panic("Unable to read input file!"); }
    fseek(f, 0, SEEK_END);
    size_t length_bytes = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*) arena_alloc(arena, length_bytes);
    fread(buffer, 1, length_bytes, f);
    fclose(f);
    return (String) {
        .data = buffer,
        .length = length_bytes
    };
}

int main(int argc, char** argv) {
    Arena arena = arena_new(2048);
    SymbolTable symbols = s_table_new();
    String main;
    bool has_main = false;
    String output;
    bool has_output = false;
    for(size_t argi = 1; argi < argc; argi += 1) {
        if(strcmp(argv[argi], "-m") == 0) {
            if(argi + 1 >= argc) { panic("Invalid CLI arguments!"); }
            main = string_wrap_nt(argv[argi + 1]);
            has_main = true;
            argi += 1;
            continue;
        } else if(strcmp(argv[argi], "-o") == 0) {
            if(argi + 1 >= argc) { panic("Invalid CLI arguments!"); }
            output = string_wrap_nt(argv[argi + 1]);
            has_output = true;
            argi += 1;
            continue;
        }
        String file = read_file(argv[argi], &arena);
        Lexer lexer = lexer_new(file);
        Parser parser = parser_new(&arena);
        Block ast = parser_parse(&parser, &lexer);
        collect_symbols(ast, &symbols, &arena);
    }
////// debugging ///////////////////////////////////////////////////////////////
    for(size_t i = 0; i < symbols.count; i += 1) {
        Symbol s = symbols.symbols[i];
        printf(" - ");
        for(size_t j = 0; j < s.path.length; j += 1) {
            if(j > 0) { printf("::"); }
            String e = s.path.elements[j];
            STRING_AS_NT(e, ent);
            printf("%s", ent);
        }
        printf("\n");
    }
////////////////////////////////////////////////////////////////////////////////
    arena_free(&arena);
    s_table_free(&symbols);
}



