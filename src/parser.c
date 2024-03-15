
#include <stdint.h>
#include "parser.h"

DEF_ARRAY_BUILDER(String)
DEF_ARRAY_BUILDER(Node)
DEF_ARRAY_BUILDER(Namespace)


Parser parser_new(Arena* a) {
    return (Parser) {
        .arena = a
    };
}

static Node parse_type(Parser* p, Lexer* l);
static Node parse_expression(Parser* p, Lexer* l, uint8_t precedence);
static Namespace* parse_usages(Parser* p, Lexer* l, size_t* pathc);
static Node parse_statement(Parser* p, Lexer* l);
static Block parse_block(Parser* p, Lexer* l);

#define PARSING_ERROR() panic("Unable to parse input!");
#define CREATE_NODE(t, variant, ...) (Node) { \
        .type = t, \
        .value = { .variant = { __VA_ARGS__ } } \
    }
#define CREATE_EMPTY_NODE(t) (Node) { .type = t }
#define ALLOC_NODE(n) alloc_node(p->arena, n)
#define CURRENT p->current
#define AT_END p->at_end
#define EXPECT(cond) if(!(cond)) PARSING_ERROR();
#define EXPECT_NEXT() if(!next_token(p, l)) PARSING_ERROR();
#define EXPECT_TYPE(t) if(CURRENT.type != t) PARSING_ERROR();
#define TRY_NEXT() next_token(p, l)
#define PARSE_TYPE() parse_type(p, l)
#define PARSE_EXPRESSION() parse_expression(p, l, P_EXPRESSION_TERMINATOR)
#define PARSE_EXPRESSION_WITH(precedence) parse_expression(p, l, precedence)
#define PARSE_STATEMENT() parse_statement(p, l)
#define PARSE_BLOCK() parse_block(p, l)

static bool next_token(Parser* p, Lexer* l) {
    bool has_next = lexer_next_filtered(l, &CURRENT);
    if(!has_next) { p->at_end = true; }
    return has_next;
}

static Node* alloc_node(Arena* a, Node n) {
    Node* p = arena_alloc(a, sizeof(Node));
    *p = n;
    return p;
}

static Node parse_identifier(Parser* p, Lexer* l, bool force_namespace) {
    String variable_name = CURRENT.content;
    if((TRY_NEXT() && (
        CURRENT.type == DOUBLE_COLON || CURRENT.type == BRACKET_OPEN
    )) || force_namespace) {
        ArrayBuilder(String) b = arraybuilder_new(String)();
        arraybuilder_push(String)(&b, variable_name);
        while(CURRENT.type == DOUBLE_COLON) {
            EXPECT_NEXT();
            EXPECT_TYPE(IDENTIFIER);
            arraybuilder_push(String)(&b, CURRENT.content);
            if(!TRY_NEXT()) { break; }
        }
        size_t template_argc = 0;
        Node* template_argv;
        if(CURRENT.type == BRACKET_OPEN) {
            EXPECT_NEXT();
            ArrayBuilder(Node) b = arraybuilder_new(Node)();
            while(CURRENT.type != BRACKET_CLOSE) {
                Node template_arg = PARSE_TYPE();
                arraybuilder_push(Node)(&b, template_arg);
            }
            TRY_NEXT();
            template_argc = b.length;
            template_argv = (Node*) arraybuilder_finish(Node)(&b, p->arena);
        }
        Namespace accessed_path = (Namespace) {
            .length = b.length,
            .elements = (String*) arraybuilder_finish(String)(&b, p->arena)
        };
        return CREATE_NODE(NAMESPACE_ACCESS_NODE, namespace_access,
            .path = accessed_path, .template_argc = template_argc,
            .template_argv = template_argv
        );
    }
    return CREATE_NODE(VARIABLE_NODE, variable,
        .name = variable_name
    );
}

static Node parse_type(Parser* p, Lexer* l) {
    switch(CURRENT.type) {
        case KEYWORD_UNIT:
            CURRENT.type = IDENTIFIER;
            return PARSE_TYPE();
        case IDENTIFIER:
            return parse_identifier(p, l, true);
        case AMPERSAND:
            EXPECT_NEXT();
            Node pointed_to = PARSE_TYPE();
            return CREATE_NODE(POINTER_TYPE_NODE, pointer_type,
                .to = ALLOC_NODE(pointed_to)
            );
    }
    PARSING_ERROR();
}

#define P_NONE 0 // operator does not have precedence
#define P_CALL_ARG 1
#define P_PARENS 1
#define P_LOGICAL_NOT 2
#define P_BITWISE_NOT 2
#define P_NEGATION 2
#define P_DEREF 2
#define P_ADDRESS_OF 2
#define P_SIZE_OF 2
#define P_TYPE_CONVERSION 3
#define P_MULTIPLICATION 4
#define P_DIVISION 4
#define P_REMAINDER 4
#define P_ADDITION 5
#define P_SUBTRACTION 5
#define P_LEFT_SHIFT 6
#define P_RIGHT_SHIFT 6
#define P_LESS_THAN 7
#define P_LESS_THAN_EQUAL 7
#define P_GREATER_THAN 7
#define P_GREATER_THAN_EQUAL 7
#define P_EQUALS 8
#define P_NOT_EQUALS 8
#define P_BITWISE_AND 9
#define P_BITWISE_XOR 10
#define P_BITWISE_OR 11
#define P_LOGICAL_AND 12
#define P_LOGICAL_OR 13
#define P_EXPRESSION_TERMINATOR 255

static uint8_t infix_operator_precedence(TokenType t) {
    switch(t) {
        case KEYWORD_AS: return P_TYPE_CONVERSION;
        case ASTERISK: return P_MULTIPLICATION;
        case SLASH: return P_DIVISION;
        case PERCENT: return P_REMAINDER;
        case PLUS: return P_ADDITION;
        case MINUS: return P_SUBTRACTION;
        case DOUBLE_LESS_THAN: return P_LEFT_SHIFT;
        case DOUBLE_GREATER_THAN: return P_RIGHT_SHIFT;
        case LESS_THAN: return P_LESS_THAN;
        case LESS_THAN_EQUAL: return P_LESS_THAN_EQUAL;
        case GREATER_THAN: return P_GREATER_THAN;
        case GREATER_THAN_EQUAL: return P_GREATER_THAN_EQUAL;
        case DOUBLE_EQUALS: return P_EQUALS;
        case NOT_EQUALS: return P_NOT_EQUALS;
        case AMPERSAND: return P_BITWISE_AND;
        case CARET: return P_BITWISE_XOR;
        case PIPE: return P_BITWISE_OR;
        case EQUALS:
        case BRACE_OPEN:
        case BRACE_CLOSE:
        case SEMICOLON:
        case PAREN_CLOSE:
            return P_EXPRESSION_TERMINATOR;
    }
    return P_NONE;
}

static Node parse_expression(Parser* p, Lexer* l, uint8_t precedence) {
    Node previous;
    bool has_previous = false;
    while(true) {
        uint8_t token_precedence = infix_operator_precedence(CURRENT.type);
        if(AT_END || token_precedence >= precedence) {
            if(!has_previous) { PARSING_ERROR(); }
            return previous;
        }
        // parse infix operators
        #define EXPECT_HAS_PREVIOUS() if(!has_previous) PARSING_ERROR();
        #define PARSE_INFIX_OPERATOR(tt, pr, nt, nv) case tt: \
                if(!has_previous) { break; } \
                Node nv##_a = previous; \
                EXPECT_NEXT(); \
                Node nv##_b = PARSE_EXPRESSION_WITH(pr); \
                previous = CREATE_NODE(nt, nv, \
                    .a = ALLOC_NODE(nv##_a), .b = ALLOC_NODE(nv##_b) \
                ); \
                has_previous = true; \
                continue;
        switch(CURRENT.type) {
            PARSE_INFIX_OPERATOR(
                PLUS, P_ADDITION, ADDITION_NODE, addition
            )
            PARSE_INFIX_OPERATOR(
                MINUS, P_SUBTRACTION, SUBTRACTION_NODE, subtraction
            )
            PARSE_INFIX_OPERATOR(
                ASTERISK, P_MULTIPLICATION, MULTIPLICATION_NODE, multiplication
            )
            PARSE_INFIX_OPERATOR(
                SLASH, P_DIVISION, DIVISION_NODE, division
            )
            PARSE_INFIX_OPERATOR(
                PERCENT, P_REMAINDER, REMAINDER_NODE, remainder
            )
            PARSE_INFIX_OPERATOR(
                AMPERSAND, P_BITWISE_AND, BITWISE_AND_NODE, bitwise_and
            )
            PARSE_INFIX_OPERATOR(
                PIPE, P_BITWISE_OR, BITWISE_OR_NODE, bitwise_or
            )
            PARSE_INFIX_OPERATOR(
                CARET, P_BITWISE_XOR, BITWISE_XOR_NODE, bitwise_xor
            )
            PARSE_INFIX_OPERATOR(
                DOUBLE_AMPERSAND, P_LOGICAL_AND, LOGICAL_AND_NODE, logical_and
            )
            PARSE_INFIX_OPERATOR(
                DOUBLE_PIPE, P_LOGICAL_OR, LOGICAL_OR_NODE, logical_or
            )
            PARSE_INFIX_OPERATOR(
                DOUBLE_EQUALS, P_EQUALS, EQUALS_NODE, equals
            )
            PARSE_INFIX_OPERATOR(
                NOT_EQUALS, P_NOT_EQUALS, NOT_EQUALS_NODE, not_equals
            )
            PARSE_INFIX_OPERATOR(
                LESS_THAN, P_LESS_THAN, LESS_THAN_NODE, less_than
            )
            PARSE_INFIX_OPERATOR(
                LESS_THAN_EQUAL, P_LESS_THAN_EQUAL, LESS_THAN_EQUAL_NODE,
                less_than_equal
            )
            PARSE_INFIX_OPERATOR(
                GREATER_THAN, P_GREATER_THAN, GREATER_THAN_NODE, greater_than
            )
            PARSE_INFIX_OPERATOR(
                GREATER_THAN_EQUAL, P_GREATER_THAN_EQUAL,
                GREATER_THAN_EQUAL_NODE, greater_than_equal
            )
            case DOUBLE_LESS_THAN:
                EXPECT_HAS_PREVIOUS();
                Node left_shift_x = previous;
                EXPECT_NEXT();
                Node left_shift_n = PARSE_EXPRESSION_WITH(P_LEFT_SHIFT);
                previous = CREATE_NODE(LEFT_SHIFT_NODE, left_shift,
                    .x = ALLOC_NODE(left_shift_x), .n = ALLOC_NODE(left_shift_n)
                );
                has_previous = true;
                continue;
            case DOUBLE_GREATER_THAN:
                EXPECT_HAS_PREVIOUS();
                Node right_shift_x = previous;
                EXPECT_NEXT();
                Node right_shift_n = PARSE_EXPRESSION_WITH(P_RIGHT_SHIFT);
                previous = CREATE_NODE(RIGHT_SHIFT_NODE, right_shift,
                    .x = ALLOC_NODE(right_shift_x), .n = ALLOC_NODE(right_shift_n)
                );
                has_previous = true;
                continue;
            case DOT:
                EXPECT_HAS_PREVIOUS();
                Node accessed_record = previous;
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                String member_name = CURRENT.content;
                TRY_NEXT();
                previous = CREATE_NODE(MEMBER_ACCESS_NODE, member_access,
                    .x = ALLOC_NODE(accessed_record), .name = member_name
                );
                has_previous = true;
                continue;
            case KEYWORD_AS:
                EXPECT_HAS_PREVIOUS();
                Node converted = previous;
                EXPECT_NEXT();
                Node target_type = PARSE_TYPE();
                previous = CREATE_NODE(TYPE_CONVERSION_NODE, type_conversion,
                    .x = ALLOC_NODE(converted), .to = ALLOC_NODE(target_type)
                );
                has_previous = true;
                continue;
        }
        // parse others
        Node node;
        switch(CURRENT.type) {
            case KEYWORD_UNIT:
                TRY_NEXT();
                node = CREATE_EMPTY_NODE(UNIT_LITERAL_NODE);
                break;
            case INTEGER:
                String integer_value = CURRENT.content;
                TRY_NEXT();
                node = CREATE_NODE(INTEGER_LITERAL_NODE, integer_literal,
                    .value = integer_value
                );
                break;
            case FLOAT:
                String float_value = CURRENT.content;
                TRY_NEXT();
                node = CREATE_NODE(FLOAT_LITERAL_NODE, float_literal,
                    .value = float_value
                );
                break;
            case STRING:
                String string_value = CURRENT.content;
                TRY_NEXT();
                node = CREATE_NODE(STRING_LITERAL_NODE, string_literal,
                    .value = string_value
                );
                break;
            case IDENTIFIER:
                node = parse_identifier(p, l, false);
                break;
            case MINUS:
                EXPECT_NEXT();
                Node mathematically_negated = PARSE_EXPRESSION_WITH(P_NEGATION);
                node = CREATE_NODE(NEGATION_NODE, negation,
                    .x = ALLOC_NODE(mathematically_negated)
                );
                break;
            case TILDE:
                EXPECT_NEXT();
                Node bitwise_negated = PARSE_EXPRESSION_WITH(P_BITWISE_NOT);
                node = CREATE_NODE(BITWISE_NOT_NODE, bitwise_not,
                    .x = ALLOC_NODE(bitwise_negated)
                );
                break;
            case EXCLAMATION_MARK:
                EXPECT_NEXT();
                Node logically_negated = PARSE_EXPRESSION_WITH(P_LOGICAL_NOT);
                node = CREATE_NODE(LOGICAL_NOT_NODE, logical_not,
                    .x = ALLOC_NODE(logically_negated)
                );
                break;
            case AT:
                EXPECT_NEXT();
                Node dereferenced = PARSE_EXPRESSION_WITH(P_DEREF);
                node = CREATE_NODE(DEREF_NODE, deref,
                    .x = ALLOC_NODE(dereferenced)
                );
                break;
            case AMPERSAND:
                EXPECT_NEXT();
                Node referenced = PARSE_EXPRESSION_WITH(P_ADDRESS_OF);
                node = CREATE_NODE(ADDRESS_OF_NODE, address_of,
                    .x = ALLOC_NODE(referenced)
                );
                break;
            case KEYWORD_SIZEOF:
                EXPECT_NEXT();
                Node queried_type = PARSE_TYPE();
                node = CREATE_NODE(SIZE_OF_NODE, size_of,
                    .t = ALLOC_NODE(queried_type)
                );
                break;
            case PAREN_OPEN:
                EXPECT_NEXT();
                node = PARSE_EXPRESSION();
                EXPECT_TYPE(PAREN_CLOSE);
                TRY_NEXT();
                break;
            default:
                PARSING_ERROR();
        }
        if(has_previous) {
            if(previous.type == CALL_NODE) {
                size_t new_argc = previous.value.call.argc + 1;
                Node* new_argv = (Node*) arena_alloc(
                    p->arena, sizeof(Node) * new_argc
                );
                memcpy(new_argv, previous.value.call.argv, new_argc - 1);
                new_argv[new_argc - 1] = node;
                previous.value.call.argc = new_argc;
                previous.value.call.argv = new_argv;
            } else {
                previous = CREATE_NODE(CALL_NODE, call,
                    .called = ALLOC_NODE(previous),
                    .argc = 1, .argv = ALLOC_NODE(node)
                );
            }
        } else {
            previous = node;
            has_previous = true;
        }
    }
}

static Namespace* parse_usages(Parser* p, Lexer* l, size_t* pathc) {
    switch(CURRENT.type) {
        case PAREN_OPEN:
            ArrayBuilder(Namespace) rb = arraybuilder_new(Namespace)();
            EXPECT_NEXT();
            while(CURRENT.type != PAREN_CLOSE) {
                size_t cpathc;
                Namespace* cpathv = parse_usages(p, l, &cpathc);
                arraybuilder_append(Namespace)(&rb, cpathc, cpathv);
            }
            EXPECT_TYPE(PAREN_CLOSE);
            TRY_NEXT();
            *pathc = rb.length;
            return (Namespace*) arraybuilder_finish(Namespace)(&rb, p->arena);
        case ASTERISK:
        case IDENTIFIER:
            bool can_contain = CURRENT.type = IDENTIFIER;
            String name = CURRENT.content;
            if(!TRY_NEXT() || CURRENT.type != DOUBLE_COLON || !can_contain) {
                String* element = (String*) arena_alloc(
                    p->arena, sizeof(String)
                );
                *element = name;
                Namespace* path = (Namespace*) arena_alloc(
                    p->arena, sizeof(Namespace)
                );
                *path = (Namespace) { .length = 1, .elements = element };
                *pathc = 1;
                return path;
            }
            EXPECT_NEXT();
            size_t fpathc;
            Namespace* fpathv = parse_usages(p, l, &fpathc);
            for(size_t fpathi = 0; fpathi < fpathc; fpathi += 1) {
                Namespace old_path = fpathv[fpathi];
                String* new_elements = (String*) arena_alloc(
                    p->arena, sizeof(String) * old_path.length + 1
                );
                new_elements[0] = name;
                memcpy(
                    new_elements + 1, old_path.elements,
                    sizeof(String) * old_path.length
                );
                fpathv[fpathi].elements = new_elements;
                fpathv[fpathi].length = old_path.length + 1;
            }
            *pathc = fpathc;
            return fpathv;
    }
}

static Node parse_statement(Parser* p, Lexer* l) {
    switch(CURRENT.type) {
        case KEYWORD_VAR:
            EXPECT_NEXT();
            EXPECT_TYPE(IDENTIFIER);
            String name = CURRENT.content;
            EXPECT_NEXT();
            Node type = PARSE_TYPE();
            EXPECT_TYPE(EQUALS);
            EXPECT_NEXT();
            Node variable_value = PARSE_EXPRESSION();
            EXPECT(AT_END || CURRENT.type == SEMICOLON);
            EXPECT_NEXT();
            return CREATE_NODE(VARIABLE_DECLARATION_NODE, variable_declaration,
                .name = name, .type = ALLOC_NODE(type),
                .value = ALLOC_NODE(variable_value)
            );
        case KEYWORD_MOD:
            EXPECT_NEXT();
            EXPECT_TYPE(IDENTIFIER);
            ArrayBuilder(String) b = arraybuilder_new(String)();
            arraybuilder_push(String)(&b, CURRENT.content);
            while(TRY_NEXT() && CURRENT.type == DOUBLE_COLON) {
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                arraybuilder_push(String)(&b, CURRENT.content);
            }
            Namespace path;
            path.length = b.length;
            path.elements = (String*) arraybuilder_finish(String)(&b, p->arena);
            return CREATE_NODE(MODULE_NODE, module,
                .path = path
            );
        case KEYWORD_USE: 
            EXPECT_NEXT();
            size_t pathc;
            Namespace* pathv = parse_usages(p, l, &pathc);
            return CREATE_NODE(USE_NODE, use, 
                .pathc = pathc,
                .pathv = pathv
            );
        case KEYWORD_PUB:
        case KEYWORD_EXT:
        case KEYWORD_FUN:
        case KEYWORD_RECORD:
            bool is_public = CURRENT.type == KEYWORD_PUB;
            if(is_public) { EXPECT_NEXT(); }
            if(CURRENT.type == KEYWORD_EXT) {
                EXPECT_NEXT();
                EXPECT_TYPE(KEYWORD_FUN);
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                ArrayBuilder(String) pb = arraybuilder_new(String)();
                arraybuilder_push(String)(&pb, CURRENT.content);
                EXPECT_NEXT();
                while(CURRENT.type == DOUBLE_COLON) {
                    EXPECT_NEXT();
                    EXPECT_TYPE(IDENTIFIER);
                    arraybuilder_push(String)(&pb, CURRENT.content);
                    EXPECT_NEXT();
                }
                Namespace path = (Namespace) {
                    .length = b.length,
                    .elements = (String*) arraybuilder_finish(String)(
                        &pb, p->arena
                    )
                };
                ArrayBuilder(String) anb = arraybuilder_new(String)();
                ArrayBuilder(Node) atb = arraybuilder_new(Node)();
                while(CURRENT.type == IDENTIFIER) {
                    arraybuilder_push(String)(&anb, CURRENT.content);
                    EXPECT_NEXT();
                    Node arg_type = PARSE_TYPE();
                    arraybuilder_push(Node)(&atb, arg_type);
                }
                Node return_type;
                if(CURRENT.type == ARROW) {
                    EXPECT_NEXT();
                    return_type = PARSE_TYPE();
                } else {
                    String* name = (String*) arena_alloc(
                        p->arena, sizeof(String)
                    );
                    *name = string_wrap_nt("unit");
                    return_type = CREATE_NODE(
                        NAMESPACE_ACCESS_NODE,
                        namespace_access,
                        .path = (Namespace) {
                            .length = 1, .elements = name
                        },
                        .template_argc = 0
                    );
                }
                EXPECT_TYPE(EQUALS);
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                String external_name = CURRENT.content;
                TRY_NEXT();
                return CREATE_NODE(EXTERNAL_FUNCTION_NODE, external_function,
                    .is_public = is_public, .path = path,
                    .argc = anb.length,
                    .argnamev = (String*) arraybuilder_finish(String)(
                        &anb, p->arena
                    ),
                    .argtypev = (Node*) arraybuilder_finish(Node)(
                        &atb, p->arena
                    ),
                    .return_type = ALLOC_NODE(return_type),
                    .external_name = external_name
                );
            } else if(CURRENT.type == KEYWORD_FUN) {
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                ArrayBuilder(String) pb = arraybuilder_new(String)();
                arraybuilder_push(String)(&pb, CURRENT.content);
                EXPECT_NEXT();
                while(CURRENT.type == DOUBLE_COLON) {
                    EXPECT_NEXT();
                    EXPECT_TYPE(IDENTIFIER);
                    arraybuilder_push(String)(&pb, CURRENT.content);
                    EXPECT_NEXT();
                }
                Namespace path = (Namespace) {
                    .length = b.length,
                    .elements = (String*) arraybuilder_finish(String)(
                        &pb, p->arena
                    )
                };
                size_t template_argc = 0;
                String* template_argnamev;
                if(CURRENT.type == BRACKET_OPEN) {
                    EXPECT_NEXT();
                    ArrayBuilder(String) anb = arraybuilder_new(String)();
                    while(CURRENT.type != BRACKET_CLOSE) {
                        EXPECT_TYPE(IDENTIFIER);
                        arraybuilder_push(String)(&anb, CURRENT.content);
                        EXPECT_NEXT();
                    }
                    EXPECT_NEXT();
                    template_argc = anb.length;
                    template_argnamev = (String*) arraybuilder_finish(String)(
                        &anb, p->arena
                    );
                }
                ArrayBuilder(String) anb = arraybuilder_new(String)();
                ArrayBuilder(Node) atb = arraybuilder_new(Node)();
                while(!AT_END && CURRENT.type == IDENTIFIER) {
                    arraybuilder_push(String)(&anb, CURRENT.content);
                    EXPECT_NEXT();
                    Node arg_type = PARSE_TYPE();
                    arraybuilder_push(Node)(&atb, arg_type);
                }
                Node return_type;
                if(CURRENT.type == ARROW) {
                    EXPECT_NEXT();
                    return_type = PARSE_TYPE();
                } else {
                    String* name = (String*) arena_alloc(
                        p->arena, sizeof(String)
                    );
                    *name = string_wrap_nt("unit");
                    return_type = CREATE_NODE(
                        NAMESPACE_ACCESS_NODE,
                        namespace_access,
                        .path = (Namespace) {
                            .length = 1, .elements = name
                        },
                        .template_argc = 0
                    );
                }
                EXPECT_TYPE(BRACE_OPEN);
                EXPECT_NEXT();
                Block body = PARSE_BLOCK();
                EXPECT_TYPE(BRACE_CLOSE);
                TRY_NEXT();
                return CREATE_NODE(FUNCTION_NODE, function,
                    .is_public = is_public, .path = path,
                    .template_argc = template_argc,
                    .template_argnamev = template_argnamev,
                    .template_argv = NULL,
                    .argc = anb.length,
                    .argnamev = (String*) arraybuilder_finish(String)(
                        &anb, p->arena
                    ),
                    .argtypev = (Node*) arraybuilder_finish(Node)(
                        &atb, p->arena
                    ),
                    .return_type = ALLOC_NODE(return_type),
                    .body = body
                );
            } else if(CURRENT.type == KEYWORD_RECORD) {
                EXPECT_NEXT();
                EXPECT_TYPE(IDENTIFIER);
                ArrayBuilder(String) pb = arraybuilder_new(String)();
                arraybuilder_push(String)(&pb, CURRENT.content);
                EXPECT_NEXT();
                while(CURRENT.type == DOUBLE_COLON) {
                    EXPECT_NEXT();
                    EXPECT_TYPE(IDENTIFIER);
                    arraybuilder_push(String)(&pb, CURRENT.content);
                    EXPECT_NEXT();
                }
                Namespace path = (Namespace) {
                    .length = pb.length,
                    .elements = (String*) arraybuilder_finish(String)(
                        &pb, p->arena
                    )
                };
                size_t template_argc = 0;
                String* template_argnamev;
                if(CURRENT.type == BRACKET_OPEN) {
                    EXPECT_NEXT();
                    ArrayBuilder(String) anb = arraybuilder_new(String)();
                    while(CURRENT.type != BRACKET_CLOSE) {
                        EXPECT_TYPE(IDENTIFIER);
                        arraybuilder_push(String)(&anb, CURRENT.content);
                        EXPECT_NEXT();
                    }
                    EXPECT_NEXT();
                    template_argc = anb.length;
                    template_argnamev = (String*) arraybuilder_finish(String)(
                        &anb, p->arena
                    );
                }
                ArrayBuilder(String) anb = arraybuilder_new(String)();
                ArrayBuilder(Node) atb = arraybuilder_new(Node)();
                while(!AT_END && CURRENT.type == IDENTIFIER) {
                    arraybuilder_push(String)(&anb, CURRENT.content);
                    EXPECT_NEXT();
                    Node arg_type = PARSE_TYPE();
                    arraybuilder_push(Node)(&atb, arg_type);
                }
                return CREATE_NODE(RECORD_NODE, record,
                    .is_public = is_public, .path = path,
                    .template_argc = template_argc,
                    .template_argnamev = template_argnamev,
                    .template_argv = NULL,
                    .argc = anb.length,
                    .argnamev = (String*) arraybuilder_finish(String)(
                        &anb, p->arena
                    ),
                    .argtypev = (Node*) arraybuilder_finish(Node)(
                        &atb, p->arena
                    )
                );
            } else { PARSING_ERROR(); }
        case KEYWORD_RETURN:
            EXPECT_NEXT();
            Node returned_value = PARSE_EXPRESSION();
            return CREATE_NODE(RETURN_VALUE_NODE, return_value,
                .x = ALLOC_NODE(returned_value)
            );
        case KEYWORD_IF:
            EXPECT_NEXT();
            Node if_condition = PARSE_EXPRESSION();
            EXPECT_TYPE(BRACE_OPEN);
            EXPECT_NEXT();
            Block if_body = PARSE_BLOCK();
            EXPECT_TYPE(BRACE_CLOSE);
            Block else_body = (Block) { .length = 0 };
            if(TRY_NEXT() && CURRENT.type == KEYWORD_ELSE) {
                EXPECT_NEXT();
                if(CURRENT.type == KEYWORD_IF) {
                    Node else_branch = PARSE_STATEMENT();
                    else_body = (Block) {
                        .length = 1, .statements = ALLOC_NODE(else_branch)
                    };
                } else if(CURRENT.type == BRACE_OPEN) {
                    EXPECT_NEXT();
                    else_body = PARSE_BLOCK();
                    EXPECT_TYPE(BRACE_CLOSE);
                    TRY_NEXT();
                } else { PARSING_ERROR(); }
            }
            return CREATE_NODE(IF_ELSE_NODE, if_else,
                .condition = ALLOC_NODE(if_condition), .if_body = if_body,
                .else_body = else_body
            );
        case KEYWORD_WHILE:
            EXPECT_NEXT();
            Node while_condition = PARSE_EXPRESSION();
            EXPECT_TYPE(BRACE_OPEN);
            EXPECT_NEXT();
            Block while_body = PARSE_BLOCK();
            EXPECT_TYPE(BRACE_CLOSE);
            TRY_NEXT();
            return CREATE_NODE(WHILE_DO_NODE, while_do,
                .condition = ALLOC_NODE(while_condition), .body = while_body
            );
    }
    Node expression = PARSE_EXPRESSION();
    if(p->current.type == EQUALS) {
        EXPECT_NEXT();
        Node value = PARSE_EXPRESSION();
        EXPECT(AT_END || CURRENT.type == SEMICOLON);
        TRY_NEXT();
        return CREATE_NODE(ASSIGNMENT_NODE, assignment,
            .to = ALLOC_NODE(expression), .value = ALLOC_NODE(value)
        );
    }
    EXPECT(AT_END || CURRENT.type == SEMICOLON);
    TRY_NEXT();
    return expression;
}

static Block parse_block(Parser* p, Lexer* l) {
    ArrayBuilder(Node) b = arraybuilder_new(Node)();
    while(CURRENT.type != BRACE_CLOSE) {
        Node statement = PARSE_STATEMENT();
        arraybuilder_push(Node)(&b, statement);
        while(CURRENT.type == SEMICOLON && TRY_NEXT()) {}
        if(AT_END) { break; }
    }
    Block result;
    result.length = b.length;
    result.statements = (Node*) arraybuilder_finish(Node)(&b, p->arena);
    return result;
}

Block parser_parse(Parser* p, Lexer* l) {
    p->at_end = false;
    if(!lexer_next_filtered(l, &p->current)) { return (Block) { .length = 0 }; }
    return parse_block(p, l);
}