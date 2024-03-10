
#include <stdio.h>
#include "parser.h"

int main() {
    String input = string_wrap_nt(
"use cock::(and balls::*)"
    );
    Lexer lexer = lexer_new(input);
    Arena arena = arena_new();
    Parser parser = parser_new(&arena);
    Block ast = parser_parse(&parser, &lexer);

    arena_free(&arena);
}



