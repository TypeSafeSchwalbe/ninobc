
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "util.h"


typedef enum {
    WHITESPACE,
    COMMENT,
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    BRACE_OPEN,
    BRACE_CLOSE,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    PAREN_OPEN,
    PAREN_CLOSE,
    EQUALS,
    ARROW,
    PLUS,
    MINUS,
    ASTERISK,
    SLASH,
    PERCENT,
    AMPERSAND,
    PIPE,
    CARET,
    DOUBLE_LESS_THAN,
    DOUBLE_GREATER_THAN,
    TILDE,
    DOUBLE_AMPERSAND,
    DOUBLE_PIPE,
    EXCLAMATION_MARK,
    DOUBLE_EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_THAN_EQUAL,
    GREATER_THAN_EQUAL,
    AT,
    DOT,
    DOUBLE_COLON,
    SEMICOLON,
    KEYWORD_MOD,
    KEYWORD_USE,
    KEYWORD_AS,
    KEYWORD_PUB,
    KEYWORD_FUN,
    KEYWORD_RETURN,
    KEYWORD_EXT,
    KEYWORD_RECORD,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_VAR,
    KEYWORD_UNIT,
    KEYWORD_SIZEOF
} TokenType;

typedef struct {
    TokenType type;
    String content;
} Token;

typedef struct {
    String src;
    size_t i;
} Lexer;

Lexer lexer_new(String src);
bool is_digit(char c);
bool is_alphanumeral(char c);
bool is_whitespace(char c);
bool is_not_line_end(char c);
bool lexer_next(Lexer* l, Token* t_out);
bool lexer_next_filtered(Lexer* l, Token* t_out);