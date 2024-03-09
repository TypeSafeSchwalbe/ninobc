
#pragma once
#include "lexer.h"


typedef struct {
    String* elements;
    size_t length;
} Namespace;


typedef enum NodeType {
    NOOP_NODE,
    INTEGER_LITERAL_NODE,
    FLOAT_LITERAL_NODE,
    STRING_LITERAL_NODE,
    VARIABLE_NODE,
    VARIABLE_DECLARATION_NODE,
    ASSIGNMENT_NODE,
    ADDITION_NODE,
    SUBTRACTION_NODE,
    MULTIPLICATION_NODE,
    DIVISION_NODE,
    REMAINDER_NODE,
    NEGATION_NODE,
    BITWISE_AND_NODE,
    BITWISE_OR_NODE,
    BITWISE_XOR_NODE,
    LEFT_SHIFT_NODE,
    RIGHT_SHIFT_NODE,
    BITWISE_NOT_NODE,
    LOGICAL_AND_NODE,
    LOGICAL_OR_NODE,
    LOGICAL_NOT_NODE,
    EQUALS_NODE,
    NOT_EQUALS_NODE,
    LESS_THAN_NODE,
    GREATER_THAN_NODE,
    LESS_THAN_EQUAL_NODE,
    GREATER_THAN_EQUAL_NODE,
    DEREF_NODE,
    ADDRESS_OF_NODE,
    MEMBER_ACCESS_NODE,
    NAMESPACE_ACCESS_NODE,
    MODULE_NODE,
    USE_NODE,
    TYPE_CONVERSION_NODE,
    FUNCTION_NODE,
    EXTERNAL_FUNCTION_NODE,
    RETURN_VALUE_NODE,
    RECORD_NODE,
    IF_ELSE_NODE,
    WHILE_DO_NODE,
    UNIT_NODE
} NodeType;

typedef struct Node Node;

typedef struct Block {
    Node* statements;
    size_t length;
} Block;

typedef struct Node {
    NodeType type;
    union {
        struct { String value; } integer_literal;
        struct { String value; } float_literal;
        struct { String value; } string_literal;
        struct { String name; } variable;
        struct { String name; Node* type; Node* value; } variable_declaration;
        struct { Node* to; Node* value; } assignment;
        struct { Node* a; Node* b; } addition;
        struct { Node* a; Node* b; } subtraction;
        struct { Node* a; Node* b; } multiplication;
        struct { Node* a; Node* b; } division;
        struct { Node* a; Node* b; } remainder;
        struct { Node* x; } negation;
        struct { Node* a; Node* b; } bitwise_and;
        struct { Node* a; Node* b; } bitwise_or;
        struct { Node* a; Node* b; } bitwise_xor;
        struct { Node* x; Node* n; } left_shift;
        struct { Node* x; Node* n; } right_shift;
        struct { Node* x; } bitwise_not;
        struct { Node* a; Node* b; } logical_and;
        struct { Node* a; Node* b; } logical_or;
        struct { Node* x; } logical_not;
        struct { Node* a; Node* b; } equals;
        struct { Node* a; Node* b; } not_equals;
        struct { Node* a; Node* b; } less_than;
        struct { Node* a; Node* b; } greater_than;
        struct { Node* a; Node* b; } less_than_equal;
        struct { Node* a; Node* b; } greater_than_equal;
        struct { Node* x; } deref;
        struct { Node* x; } address_of;
        struct { Node* x; String name; } member_access;
        struct {
            Namespace path; size_t template_argc; Node* template_argv;
        } namespace_access;
        struct { Namespace path; } module;
        struct { size_t pathc; Namespace* pathv; } use;
        struct { Node* x; Node* to; } type_conversion;
        struct {
            bool is_public;
            Namespace path;
            size_t template_argc; String* template_argnamev;
            size_t argc; String* argnamev; Node* argtypev;
            Node* return_type;
            Block body;
        } function;
        struct {
            bool is_public;
            Namespace path;
            size_t argc; String* argnamev; Node* argtypev;
            Node* return_type;
            String external_name;
        } external_function;
        struct { Node* x; } return_value;
        struct {
            bool is_public;
            Namespace path;
            size_t template_argc; String* template_argnamev;
            size_t argc; String* argnamev; Node* argtypev;
        } record;
        struct { Node* condition; Block if_body; Block else_body; } if_else;
        struct { Node* condition; Block body; } while_do;
    } value;
} Node;


typedef struct {
    Arena* arena;
    Token current;
} Parser;

Parser parser_new(Arena* a);
Block parser_parse(Parser* p, Lexer* l);