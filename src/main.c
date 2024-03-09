
#include <stdio.h>
#include "parser.h"

int main() {
    String input = string_wrap_nt("1 + 1 = 3");
    Lexer lexer = lexer_new(input);
    Arena arena = arena_new();
    Parser parser = parser_new(&arena);
    Block ast = parser_parse(&parser, &lexer);
}