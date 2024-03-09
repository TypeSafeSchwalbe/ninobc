
#include "lexer.h"

Lexer lexer_new(String src) {
    return (Lexer) {
        .i = 0,
        .src = src
    };
}

static bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

static bool is_alphanumeral(char c) {
    return ('0' <= c && c <= '9')
        || ('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'A')
        || c == '_';
}

static bool is_whitespace(char c) {
    switch(c) {
        case 9: // horizontal tab
        case 10: // line feed
        case 13: // carriage feed
        case 32: // space
            return true;
    }
    return false;
}

static bool is_not_line_end(char c) {
    switch(c) {
        case 10: // line feed
        case 13: // carriage feed
            return false;
    }
    return true;
}

bool lexer_next(Lexer* l, Token* t_out) {
    if(l->i >= l->src.length) { return false; }
    char start = string_char_at(l->src, l->i);
    // integers and floats
    if(start == '+' || start == '-' || is_digit(start)) {
        size_t end = l->i + 1;
        TokenType type = INTEGER;
        while(end < l->src.length
            && is_digit(string_char_at(l->src, end))) { end += 1; }
        if(string_char_at(l->src, end) == '.') {
            type = FLOAT;
            end += 1;
            while(end < l->src.length
                && is_digit(string_char_at(l->src, end))) { end += 1; }
        }
        *t_out = (Token) {
            .content = string_slice(l->src, l->i, end),
            .type = type
        };
        l->i = end;
        return true;
    }
    // other complex tokens
    #define lex_while(f, t) if(f(start)) { \
            size_t end = l->i; \
            while(end < l->src.length \
                && f(string_char_at(l->src, end))) { end += 1; } \
            *t_out = (Token) { \
                .content = string_slice(l->src, l->i, end), \
                .type = t \
            }; \
            l->i = end; \
            return true; \
        }
    // lex_while(is_alphanumeral, IDENTIFIER)
    lex_while(is_whitespace, WHITESPACE)
    // if(l->src.length >= l->i + 2 && string_starts_with(
    //     string_slice(l->src, l->i, l->src.length), string_wrap_nt("//")
    // )) {
    //     lex_while(is_not_line_end, COMMENT)
    // }
    // multi-char tokens
    #define lex_multi(s, t) { \
            String c = string_wrap_nt(s); \
            if(l->src.length >= l->i + c.length && string_starts_with( \
                string_slice(l->src, l->i, l->src.length), c \
            )) { \
                *t_out = (Token) { .content = c, .type = t }; \
                l->i += c.length; \
                return true; \
            } \
        }
    lex_multi("->", ARROW)
    lex_multi("<<", DOUBLE_LESS_THAN)
    lex_multi(">>", DOUBLE_GREATER_THAN)
    lex_multi("&&", DOUBLE_AMPERSAND)
    lex_multi("||", DOUBLE_PIPE)
    lex_multi("==", DOUBLE_EQUALS)
    lex_multi("!=", NOT_EQUALS)
    lex_multi("<=", LESS_THAN_EQUAL)
    lex_multi(">=", GREATER_THAN_EQUAL)
    lex_multi("::", DOUBLE_COLON)
    lex_multi("mod", KEYWORD_MOD)
    lex_multi("use", KEYWORD_USE)
    lex_multi("as", KEYWORD_AS)
    lex_multi("pub", KEYWORD_PUB)
    lex_multi("fun", KEYWORD_RETURN)
    lex_multi("return", KEYWORD_RETURN)
    lex_multi("ext", KEYWORD_EXT)
    lex_multi("record", KEYWORD_RECORD)
    lex_multi("template", KEYWORD_TEMPLATE)
    lex_multi("if", KEYWORD_IF)
    lex_multi("else", KEYWORD_ELSE)
    lex_multi("while", KEYWORD_WHILE)
    lex_multi("unit", KEYWORD_UNIT)
    // single-char tokens
    #define lex_single(c, t) \
        case c: \
            *t_out = (Token) { \
                .content = string_slice(l->src, l->i, l->i + 1), \
                .type = t \
            }; \
            l->i += 1; \
            return true;
    switch(start) {
        lex_single('{', BRACE_OPEN)
        lex_single('}', BRACE_CLOSE)
        lex_single('[', BRACKET_OPEN)
        lex_single(']', BRACE_CLOSE)
        lex_single('(', PAREN_OPEN)
        lex_single(')', PAREN_CLOSE)
        lex_single('=', EQUALS)
        lex_single('+', PLUS)
        lex_single('-', MINUS)
        lex_single('*', ASTERISK)
        lex_single('/', SLASH)
        lex_single('%', PERCENT)
        lex_single('&', AMPERSAND)
        lex_single('|', PIPE)
        lex_single('^', CARET)
        lex_single('~', TILDE)
        lex_single('!', EXCLAMATION_MARK)
        lex_single('<', LESS_THAN)
        lex_single('>', GREATER_THAN)
        lex_single('@', AT)
        lex_single('.', DOT)
    }
    panic("Unable to tokenize input!");
}