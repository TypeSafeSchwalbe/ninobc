
#include <stdio.h>
#include "parser.h"
#include "symbols.h"
#include "codegen.h"

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

DEF_ARRAY_BUILDER(String)

static bool parse_path(String src, Arena* arena, Namespace* out_path) {
    ArrayBuilder(String) pb = arraybuilder_new(String)();
    size_t anchor = 0;
    for(size_t i = 0;; i += 1) {
        if(i >= src.length) {
            arraybuilder_push(String)(&pb, string_slice(src, anchor, i));
            break;
        }
        char c = string_char_at(src, i);
        if(c == ':') {
            if(i - anchor == 0) {
                arraybuilder_discard(String)(&pb);
                return false;
            }
            if(i + 2 >= src.length || string_char_at(src, i) != ':') {
                arraybuilder_discard(String)(&pb);
                return false;
            }
            arraybuilder_push(String)(&pb, string_slice(src, anchor, i));
            i += 1;
            anchor = i + 1;
        } else if(!is_alphanumeral(c)) {
            arraybuilder_discard(String)(&pb);
            return false;
        }
    }
    *out_path = (Namespace) {
        .length = pb.length,
        .elements = (String*) arraybuilder_finish(String)(&pb, arena),
    };
    return true;
}

static void write_file(const char* path, size_t n, const char* data) {
    FILE* f = fopen(path, "w");
    if(f == NULL) { return panic("Unable to write output file!"); }
    fwrite(data, sizeof(char), n, f);
    fclose(f);
}

int main(int argc, const char** argv) {
    Arena arena = arena_new(2048);
    SymbolTable symbols = s_table_new();
    String main;
    bool has_main = false;
    const char* output_file;
    bool has_output_file = false;
    for(size_t argi = 1; argi < argc; argi += 1) {
        if(strcmp(argv[argi], "-m") == 0) {
            if(argi + 1 >= argc) { panic("Invalid CLI arguments!"); }
            main = string_wrap_nt(argv[argi + 1]);
            has_main = true;
            argi += 1;
            continue;
        } else if(strcmp(argv[argi], "-o") == 0) {
            if(argi + 1 >= argc) { panic("Invalid CLI arguments!"); }
            output_file = argv[argi + 1];
            has_output_file = true;
            argi += 1;
            continue;
        }
        String file = read_file(argv[argi], &arena);
        Lexer lexer = lexer_new(file);
        Parser parser = parser_new(&arena);
        Block ast = parser_parse(&parser, &lexer);
        collect_symbols(ast, &symbols, &arena);
    }
    if(!has_output_file) { panic("No output file path specified!"); }
    for(size_t symboli = 0; symboli < symbols.count; symboli += 1) {
        Symbol* symbol = symbols.symbols + symboli;
        if(symbol_targc(symbol) > 0) { continue; }
        symbol_find_variant(symbol, 0, NULL, &symbols, &arena);
    }
    Namespace main_path;
    if(has_main && !parse_path(main, &arena, &main_path)) {
        panic("Main path is invalid!");
    }
    StringBuilder output = generate_code(&symbols, has_main? &main_path : NULL);
    write_file(output_file, output.length, output.buffer);
    stringbuilder_free(&output);
    arena_free(&arena);
    s_table_free(&symbols);
}



