
#include "parser.h"

Parser parser_new(Arena* a) {
    return (Parser) {
        .arena = a
    };
}

static Node parse_type(Parser* p, Lexer* l);
static Node parse_expression(Parser* p, Lexer* l);
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
#define EXPECT(cond) if(!(cond)) PARSING_ERROR();
#define EXPECT_NEXT() if(!lexer_next_filtered(l, &CURRENT)) PARSING_ERROR();
#define EXPECT_TYPE(t) if(CURRENT.type != t) PARSING_ERROR();
#define TRY_NEXT() lexer_next_filtered(l, &CURRENT)
#define PARSE_TYPE() parse_statement(p, l)
#define PARSE_EXPRESSION() parse_expression(p, l)
#define PARSE_STATEMENT() parse_statement(p, l)

static Node* alloc_node(Arena* a, Node n) {
    Node* p = arena_alloc(a, sizeof(Node));
    *p = n;
    return p;
}

static Node parse_type(Parser* p, Lexer* l) {
    panic("TODO!");
}

static Node parse_expression(Parser* p, Lexer* l) {
    panic("TODO!");
}

DEF_ARRAY_BUILDER(String)

static Node parse_statement(Parser* p, Lexer* l) {
    switch(CURRENT.type) {
        case SEMICOLON:
            return CREATE_EMPTY_NODE(NOOP_NODE);
        case KEYWORD_VAR:
            EXPECT_NEXT();
            EXPECT_TYPE(IDENTIFIER);
            String name = CURRENT.content;
            EXPECT_NEXT();
            Node type = PARSE_TYPE();
            EXPECT_TYPE(EQUALS);
            Node value = PARSE_EXPRESSION();
            return CREATE_NODE(VARIABLE_DECLARATION_NODE, variable_declaration,
                .name = name, .type = ALLOC_NODE(type),
                .value = ALLOC_NODE(value)
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
            panic("TODO!");
        case KEYWORD_PUB:
            panic("TODO!");
            case KEYWORD_EXT:
                panic("TODO!"); 
                case KEYWORD_FUN:
                    panic("TODO!");
                case KEYWORD_RECORD:
                    panic("TODO!");
        case KEYWORD_RETURN:
            panic("TODO!");
        case KEYWORD_IF:
            panic("TODO!");
        case KEYWORD_WHILE:
            panic("TODO!");
    }
    Node expression = PARSE_EXPRESSION();
    if(p->current.type == EQUALS) {
        EXPECT_NEXT();
        Node value = PARSE_EXPRESSION();
        return CREATE_NODE(ASSIGNMENT_NODE, assignment,
            .to = ALLOC_NODE(expression), .value = ALLOC_NODE(value)
        );
    }
    return expression;
}

DEF_ARRAY_BUILDER(Node)

static Block parse_block(Parser* p, Lexer* l) {
    ArrayBuilder(Node) b = arraybuilder_new(Node)();
    while(CURRENT.type != BRACE_CLOSE) {
        Node statement = PARSE_STATEMENT();
        EXPECT(CURRENT.type == SEMICOLON || CURRENT.type == BRACE_CLOSE);
        if(statement.type == NOOP_NODE) { continue; }
        arraybuilder_push(Node)(&b, statement);
    }
    Block result;
    result.length = b.length;
    result.statements = (Node*) arraybuilder_finish(Node)(&b, p->arena);
    return result;
}

Block parser_parse(Parser* p, Lexer* l) {
    if(!lexer_next_filtered(l, &p->current)) { return (Block) { .length = 0 }; }
    return parse_block(p, l);
}