
#include "lexer.h"

Lexer lexer_new(String src) {
    return (Lexer) {
        .i = 0,
        .src = src
    };
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool is_alphanumeral(char c) {
    return ('0' <= c && c <= '9')
        || ('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'Z')
        || c == '_';
}

bool is_whitespace(char c) {
    switch(c) {
        case 9: // horizontal tab
        case 10: // line feed
        case 13: // carriage feed
        case 32: // space
            return true;
    }
    return false;
}

bool is_not_line_end(char c) {
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
    // string literal
    if(start == '"') {
        size_t end = l->i + 1;
        bool escaped = false;
        while(end < l->src.length
            && (string_char_at(l->src, end) != '\"' || escaped)) {
            escaped = !escaped && string_char_at(l->src, end) == '\\';
            end += 1;
        }
        end += 1;
        *t_out = (Token) {
            .content = string_slice(l->src, l->i, end),
            .type = STRING
        };
        l->i = end;
        return true;
    }
    // integers and floats
    bool is_sign = start == '+' || start == '-';
    bool is_number = (is_sign && l->src.length >= l->i + 2
        && is_digit(string_char_at(l->src, l->i + 1)))
        || is_digit(start);
    if(is_number) {
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
    // keywords
    #define LEX_KEYWORD(k, t) { \
            String c = string_wrap_nt(k); \
            bool swk = l->src.length >= l->i + c.length && string_starts_with( \
                string_slice(l->src, l->i, l->src.length), c \
            ); \
            bool valid_ending = l->src.length == l->i + c.length \
                || !is_alphanumeral(string_char_at(l->src, l->i + c.length)); \
            if(swk && valid_ending) { \
                *t_out = (Token) { .content = c, .type = t }; \
                l->i += c.length; \
                return true; \
            } \
        }
    LEX_KEYWORD("mod", KEYWORD_MOD)
    LEX_KEYWORD("use", KEYWORD_USE)
    LEX_KEYWORD("as", KEYWORD_AS)
    LEX_KEYWORD("pub", KEYWORD_PUB)
    LEX_KEYWORD("fun", KEYWORD_FUN)
    LEX_KEYWORD("return", KEYWORD_RETURN)
    LEX_KEYWORD("ext", KEYWORD_EXT)
    LEX_KEYWORD("record", KEYWORD_RECORD)
    LEX_KEYWORD("if", KEYWORD_IF)
    LEX_KEYWORD("else", KEYWORD_ELSE)
    LEX_KEYWORD("while", KEYWORD_WHILE)
    LEX_KEYWORD("var", KEYWORD_VAR)
    LEX_KEYWORD("unit", KEYWORD_UNIT)
    LEX_KEYWORD("sizeof", KEYWORD_SIZEOF)
    // other complex tokens
    #define LEX_WHILE(f, t) if(f(start)) { \
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
    LEX_WHILE(is_alphanumeral, IDENTIFIER)
    LEX_WHILE(is_whitespace, WHITESPACE)
    if(l->src.length >= l->i + 2 && string_starts_with(
        string_slice(l->src, l->i, l->src.length), string_wrap_nt("//")
    )) {
        LEX_WHILE(is_not_line_end, COMMENT)
    }
    // other multi-char tokens
    #define LEX_MULTI(s, t) { \
            String c = string_wrap_nt(s); \
            if(l->src.length >= l->i + c.length && string_starts_with( \
                string_slice(l->src, l->i, l->src.length), c \
            )) { \
                *t_out = (Token) { .content = c, .type = t }; \
                l->i += c.length; \
                return true; \
            } \
        }
    LEX_MULTI("->", ARROW)
    LEX_MULTI("<<", DOUBLE_LESS_THAN)
    LEX_MULTI(">>", DOUBLE_GREATER_THAN)
    LEX_MULTI("&&", DOUBLE_AMPERSAND)
    LEX_MULTI("||", DOUBLE_PIPE)
    LEX_MULTI("==", DOUBLE_EQUALS)
    LEX_MULTI("!=", NOT_EQUALS)
    LEX_MULTI("<=", LESS_THAN_EQUAL)
    LEX_MULTI(">=", GREATER_THAN_EQUAL)
    LEX_MULTI("::", DOUBLE_COLON)
    // single-char tokens
    #define LEX_SINGLE(c, t) \
        case c: \
            *t_out = (Token) { \
                .content = string_slice(l->src, l->i, l->i + 1), \
                .type = t \
            }; \
            l->i += 1; \
            return true;
    switch(start) {
        LEX_SINGLE('{', BRACE_OPEN)
        LEX_SINGLE('}', BRACE_CLOSE)
        LEX_SINGLE('[', BRACKET_OPEN)
        LEX_SINGLE(']', BRACKET_CLOSE)
        LEX_SINGLE('(', PAREN_OPEN)
        LEX_SINGLE(')', PAREN_CLOSE)
        LEX_SINGLE('=', EQUALS)
        LEX_SINGLE('+', PLUS)
        LEX_SINGLE('-', MINUS)
        LEX_SINGLE('*', ASTERISK)
        LEX_SINGLE('/', SLASH)
        LEX_SINGLE('%', PERCENT)
        LEX_SINGLE('&', AMPERSAND)
        LEX_SINGLE('|', PIPE)
        LEX_SINGLE('^', CARET)
        LEX_SINGLE('~', TILDE)
        LEX_SINGLE('!', EXCLAMATION_MARK)
        LEX_SINGLE('<', LESS_THAN)
        LEX_SINGLE('>', GREATER_THAN)
        LEX_SINGLE('@', AT)
        LEX_SINGLE('.', DOT)
        LEX_SINGLE(';', SEMICOLON)
    }
    panic("Unable to tokenize input!");
}

bool lexer_next_filtered(Lexer* l, Token* t_out) {
    Token t;
    while(lexer_next(l, &t)) {
        if(t.type == WHITESPACE || t.type == COMMENT) { continue; }
        *t_out = t;
        return true;
    }
    return false;
}