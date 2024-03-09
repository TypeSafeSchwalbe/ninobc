
#include <stdio.h>
#include "lexer.h"

int main() {
    String input = string_wrap_nt("1 + 1 = 3");
    Lexer lexer = lexer_new(input);
    Token current_token;
    while(lexer_next(&lexer, &current_token)) {
        STRING_AS_NT(current_token.content, content)
        printf("'%s' (%d)\n", content, current_token.type);
    }
    return 0;
}