
#include "symbols.h"


static Node* alloc_node(Node n, Arena* arena) {
    Node* r = (Node*) arena_alloc(arena, sizeof(Node));
    *r = n;
    return r;
}

#define ALLOC_NODE(nv) alloc_node((nv), arena)


typedef struct {
    size_t entry_count;
    String* entry_names;
    Node* entry_nodes;
    size_t entry_bsize;
} TemplateArgs;

static TemplateArgs targs_new() {
    TemplateArgs targs;
    targs.entry_bsize = 2;
    targs.entry_count = 0;
    targs.entry_names = (String*) malloc(sizeof(String) * targs.entry_bsize);
    targs.entry_nodes = (Node*) malloc(sizeof(Node) * targs.entry_bsize);
    return targs;
}

static void targs_add(TemplateArgs* targs, String name, Node value) {
    if(targs->entry_count + 1 > targs->entry_bsize) {
        targs->entry_bsize *= 2;
        targs->entry_names = (String*) realloc(
            targs->entry_names, sizeof(String) * targs->entry_bsize
        );
        targs->entry_nodes = (Node*) realloc(
            targs->entry_nodes, sizeof(Node) * targs->entry_bsize
        );
    }
    targs->entry_names[targs->entry_count] = name;
    targs->entry_nodes[targs->entry_count] = value;
    targs->entry_count += 1;
}

static Node* targs_lookup(TemplateArgs* targs, String name) {
    for(size_t entryi = 0; entryi < targs->entry_count; entryi += 1) {
        if(!string_eq(targs->entry_names[entryi], name)) { continue; }
        return &targs->entry_nodes[entryi];
    }
    return NULL;
}

static void targs_free(TemplateArgs* targs) {
    free(targs->entry_names);
    free(targs->entry_nodes);
}


static Node monomorphize_node(
    Node* n, Symbol* symbol, SymbolTable* symbols, Arena* arena, 
    TemplateArgs* targs
);

DEF_ARRAY_BUILDER(Node);

static Block monomorphize_block(
    Block b, Symbol* symbol, SymbolTable* symbols, Arena* arena,
    TemplateArgs* targs
) {
    ArrayBuilder(Node) sb = arraybuilder_new(Node)();
    for(size_t si = 0; si < b.length; si += 1) {
        arraybuilder_push(Node)(&sb, monomorphize_node(
            b.statements + si, symbol, symbols, arena, targs
        ));
    }
    return (Block) {
        .length = sb.length,
        .statements = (Node*) arraybuilder_finish(Node)(&sb, arena)
    };
}

#include <stdio.h>

static bool expand_path(
    Namespace accessed_path, Symbol* symbol, SymbolTable* symbols, Arena* arena,
    Namespace* expanded
) {
    for(size_t symboli = 0; symboli < symbols->count; symboli += 1) {
        Symbol* csymbol = symbols->symbols + symboli;
        if(csymbol->path.length != symbol->defined_in.length
            + accessed_path.length) { continue; }
        bool matches = true;
        for(size_t ei = 0; ei < symbol->defined_in.length; ei += 1) {
            if(string_eq(
                csymbol->path.elements[ei],
                symbol->defined_in.elements[ei]
            )) { continue; }
            matches = false;
            break;
        }
        if(!matches) { continue; }
        for(size_t ei = 0; ei < accessed_path.length; ei += 1) {
            if(string_eq(
                csymbol->path.elements[symbol->defined_in.length + ei],
                accessed_path.elements[ei]
            )) { continue; }
            matches = false;
            break;
        }
        if(!matches) { continue; }
        *expanded = csymbol->path;
        return true;
    }
    signed long long int usagei;
    for(usagei = symbol->used_path_count - 1; usagei >= 0; usagei -= 1) {
        Namespace used_path = symbol->used_paths[usagei];
        String last_part = used_path.elements[used_path.length - 1];
        if(string_eq(last_part, string_wrap_nt("*"))) {
            for(size_t symboli = 0; symboli < symbols->count; symboli += 1) {
                Namespace cpath = symbols->symbols[symboli].path;
                if(cpath.length < used_path.length + accessed_path.length - 1) {
                    continue;
                }
                bool matches = true;
                for(size_t ei = 0; ei < used_path.length - 1; ei += 1) {
                    if(string_eq(
                        cpath.elements[ei],
                        used_path.elements[ei]
                    )) { continue; }
                    matches = false;
                    break;
                }
                if(!matches) { continue; }
                for(size_t ei = 0; ei < accessed_path.length; ei += 1) {
                    if(string_eq(
                        cpath.elements[used_path.length - 1 + ei],
                        accessed_path.elements[ei]
                    )) { continue; }
                    matches = false;
                    break;
                }
                if(!matches) { continue; }
                *expanded = cpath;
                return true;
            }
        }
        if(!string_eq(last_part, accessed_path.elements[0])) { continue; }
        expanded->length = used_path.length + accessed_path.length - 1;
        expanded->elements = (String*) arena_alloc(
            arena, expanded->length * sizeof(String)
        );
        memcpy(
            expanded->elements, used_path.elements,
            used_path.length * sizeof(String)
        );
        memcpy(
            expanded->elements + used_path.length, accessed_path.elements + 1,
            (accessed_path.length - 1) * sizeof(String)
        );
        return true;
    }
    return false;
}

static Node monomorphize_node(
    Node* n, Symbol* symbol, SymbolTable* symbols, Arena* arena, 
    TemplateArgs* targs
) {
    #define MONOMORPHIZE_MONOOP(tn, vn, x, ...) case tn: \
        return (Node) { .type = tn, .value = { .vn = { \
            .x = ALLOC_NODE(monomorphize_node( \
                n->value.vn.x, symbol, symbols, arena, targs \
            )), \
            __VA_ARGS__ \
        } } };
    #define MONOMORPHIZE_BIOP(tn, vn, a, b, ...) case tn: \
        return (Node) { .type = tn, .value = { .vn = { \
            .a = ALLOC_NODE(monomorphize_node( \
                n->value.vn.a, symbol, symbols, arena, targs \
            )), \
            .b = ALLOC_NODE(monomorphize_node( \
                n->value.vn.b, symbol, symbols, arena, targs \
            )), \
            __VA_ARGS__ \
        } } };
    switch(n->type) {
        case UNIT_LITERAL_NODE:
        case INTEGER_LITERAL_NODE:
        case FLOAT_LITERAL_NODE:
        case STRING_LITERAL_NODE:
        case BOOLEAN_LITERAL_NODE:
        case MODULE_NODE:
        case USE_NODE:
        case EXTERNAL_FUNCTION_NODE:
            return *n;
        MONOMORPHIZE_BIOP(ASSIGNMENT_NODE, assignment, to, value)
        MONOMORPHIZE_BIOP(ADDITION_NODE, addition, a, b)
        MONOMORPHIZE_BIOP(SUBTRACTION_NODE, subtraction, a, b)
        MONOMORPHIZE_BIOP(MULTIPLICATION_NODE, multiplication, a, b)
        MONOMORPHIZE_BIOP(DIVISION_NODE, division, a, b)
        MONOMORPHIZE_BIOP(REMAINDER_NODE, remainder, a, b)
        MONOMORPHIZE_MONOOP(NEGATION_NODE, negation, x)
        MONOMORPHIZE_BIOP(BITWISE_AND_NODE, bitwise_and, a, b)
        MONOMORPHIZE_BIOP(BITWISE_OR_NODE, bitwise_or, a, b)
        MONOMORPHIZE_BIOP(BITWISE_XOR_NODE, bitwise_xor, a, b)
        MONOMORPHIZE_BIOP(LEFT_SHIFT_NODE, left_shift, x, n)
        MONOMORPHIZE_BIOP(RIGHT_SHIFT_NODE, right_shift, x, n)
        MONOMORPHIZE_MONOOP(BITWISE_NOT_NODE, bitwise_not, x)
        MONOMORPHIZE_BIOP(LOGICAL_AND_NODE, logical_and, a, b)
        MONOMORPHIZE_BIOP(LOGICAL_OR_NODE, logical_or, a, b)
        MONOMORPHIZE_MONOOP(LOGICAL_NOT_NODE, logical_not, x)
        MONOMORPHIZE_BIOP(EQUALS_NODE, equals, a, b)
        MONOMORPHIZE_BIOP(NOT_EQUALS_NODE, not_equals, a, b)
        MONOMORPHIZE_BIOP(LESS_THAN_NODE, less_than, a, b)
        MONOMORPHIZE_BIOP(GREATER_THAN_NODE, greater_than, a, b)
        MONOMORPHIZE_BIOP(LESS_THAN_EQUAL_NODE, less_than_equal, a, b)
        MONOMORPHIZE_BIOP(GREATER_THAN_EQUAL_NODE, greater_than_equal, a, b)
        MONOMORPHIZE_MONOOP(DEREF_NODE, deref, x)
        MONOMORPHIZE_MONOOP(ADDRESS_OF_NODE, deref, x)
        MONOMORPHIZE_MONOOP(SIZE_OF_NODE, size_of, t)
        MONOMORPHIZE_BIOP(TYPE_CONVERSION_NODE, type_conversion, x, to)
        MONOMORPHIZE_MONOOP(POINTER_TYPE_NODE, pointer_type, to)
        MONOMORPHIZE_BIOP(
            VARIABLE_DECLARATION_NODE, variable_declaration, 
            type, value,
            .name = n->value.variable_declaration.name
        )
        MONOMORPHIZE_MONOOP(
            MEMBER_ACCESS_NODE, member_access, x,
            .name = n->value.member_access.name
        )
        MONOMORPHIZE_MONOOP(
            IF_ELSE_NODE, if_else, condition,
            .if_body = monomorphize_block(
                n->value.if_else.if_body, symbol, symbols, arena, targs
            ),
            .else_body = monomorphize_block(
                n->value.if_else.else_body, symbol, symbols, arena, targs
            )
        )
        MONOMORPHIZE_MONOOP(
            WHILE_DO_NODE, while_do, condition,
            .body = monomorphize_block(
                n->value.while_do.body, symbol, symbols, arena, targs
            )
        )
        case VARIABLE_NODE: {
            Namespace var_as_path = (Namespace) {
                .elements = &n->value.variable.name,
                .length = 1
            };
            Namespace expanded_var_path;
            if(!expand_path(
                var_as_path, symbol, symbols, arena, &expanded_var_path
            )) { return *n; }
            return (Node) {
                .type = NAMESPACE_ACCESS_NODE,
                .value = { .namespace_access = {
                    .path = expanded_var_path,
                    .variant = 0,
                    .template_argc = 0,
                    .template_argv = NULL
                } }
            };
        }
        case FUNCTION_NODE: {
            size_t targc = n->value.function.template_argc;
            Node* targv = (Node*) arena_alloc(arena, sizeof(Node) * targc);
            for(size_t argi = 0; argi < targc; argi += 1) {
                Node* targiv = targs_lookup(
                    targs, n->value.function.template_argnamev[argi]
                );
                if(targiv == NULL) { panic("SHOULD HAVE A VALUE???"); }
                targv[argi] = *targiv;
            }
            size_t argc = n->value.function.argc;
            Node* argtypev = (Node*) arena_alloc(arena, sizeof(Node) * argc);
            for(size_t argi = 0; argi < argc; argi += 1) {
                argtypev[argi] = monomorphize_node(
                    n->value.function.argtypev + argi, symbol, symbols, arena,
                    targs
                );
            }
            return (Node) {
                .type = FUNCTION_NODE,
                .value = { .function = {
                    .is_public = n->value.function.is_public,
                    .path = n->value.function.path,
                    .template_argc = targc,
                    .template_argnamev = n->value.function.template_argnamev,
                    .template_argv = targv,
                    .argc = argc,
                    .argnamev = n->value.function.argnamev,
                    .argtypev = argtypev,
                    .return_type = ALLOC_NODE(monomorphize_node(
                        n->value.function.return_type, symbol, symbols, arena,
                        targs
                    )),
                    .body = monomorphize_block(
                        n->value.function.body, symbol, symbols, arena, targs
                    )
                } }
            };
        }
        case RECORD_NODE: {
            size_t targc = n->value.record.template_argc;
            Node* targv = (Node*) arena_alloc(arena, sizeof(Node) * targc);
            for(size_t argi = 0; argi < targc; argi += 1) {
                Node* targiv = targs_lookup(
                    targs, n->value.record.template_argnamev[argi]
                );
                if(targiv == NULL) { panic("SHOULD HAVE A VALUE???"); }
                targv[argi] = *targiv;
            }
            size_t argc = n->value.record.argc;
            Node* argtypev = (Node*) arena_alloc(arena, sizeof(Node) * argc);
            for(size_t argi = 0; argi < argc; argi += 1) {
                argtypev[argi] = monomorphize_node(
                    n->value.record.argtypev + argi, symbol, symbols, arena,
                    targs
                );
            }
            return (Node) {
                .type = RECORD_NODE,
                .value = { .record = {
                    .is_public = n->value.record.is_public,
                    .path = n->value.record.path,
                    .template_argc = targc,
                    .template_argnamev = n->value.record.template_argnamev,
                    .template_argv = targv,
                    .argc = argc,
                    .argnamev = n->value.record.argnamev,
                    .argtypev = argtypev
                } }
            };
        }
        case RETURN_VALUE_NODE: {
            bool has_value = n->value.return_value.has_value;
            return (Node) {
                .type = RETURN_VALUE_NODE,
                .value = { .return_value = {
                    .has_value = has_value,
                    .value = has_value? ALLOC_NODE(monomorphize_node(
                        n->value.return_value.value, symbol, symbols, arena, 
                        targs
                    )) : NULL
                } }
            };
        }
        case CALL_NODE:
            size_t call_argc = n->value.call.argc;
            Node* call_argv = (Node*) arena_alloc(
                arena, sizeof(Node) * call_argc
            );
            for(size_t argi = 0; argi < call_argc; argi += 1) {
                call_argv[argi] = monomorphize_node(
                    n->value.call.argv + argi, symbol, symbols, arena, targs
                );
            }
            return (Node) {
                .type = CALL_NODE,
                .value = { .call = {
                    .called = ALLOC_NODE(monomorphize_node(
                        n->value.call.called, symbol, symbols, arena, targs
                    )),
                    .argc = call_argc,
                    .argv = call_argv
                } }
            };
        case NAMESPACE_ACCESS_NODE:
            Namespace accessed_path = n->value.namespace_access.path;
            if(accessed_path.length == 1) {
                Node* targ;
                if(targ = targs_lookup(
                    targs, accessed_path.elements[0]
                )) { return *targ; }
            }
            Namespace expanded_accessed_path;
            if(expand_path(
                accessed_path, symbol, symbols, arena, &expanded_accessed_path
            )) {
                accessed_path = expanded_accessed_path;
            }
            Symbol* symbol;
            if(!(symbol = s_table_lookup(symbols, accessed_path))) {
                return *n;
            }
            size_t targc = n->value.namespace_access.template_argc;
            Node* node_targv = n->value.namespace_access.template_argv;
            Node targv[targc];
            for(size_t argi = 0; argi < targc; argi += 1) {
                targv[argi] = monomorphize_node(
                    node_targv + argi, symbol, symbols, arena, targs
                );
            }
            size_t variant = symbol_find_variant(
                symbol, targc, targv, symbols, arena
            );
            Node access_node = (Node) {
                .type = NAMESPACE_ACCESS_NODE,
                .value = { .namespace_access = {
                    .path = accessed_path,
                    //.template_argc = targc,
                    //.template_argv = targv,
                    .variant = variant
                } }
            };
            return access_node;
    }
}


static bool template_arg_eq(Node* a, Node* b) {
    if(a->type != b->type) { return false; }
    switch(a->type) {
        case NAMESPACE_ACCESS_NODE:
            Namespace path_a = a->value.namespace_access.path;
            Namespace path_b = b->value.namespace_access.path;
            if(path_a.length != path_b.length) { return false; }
            for(size_t elementi = 0; elementi < path_a.length; elementi += 1) {
                if(!string_eq(
                    path_a.elements[elementi], path_b.elements[elementi]
                )) { return false; }
            }
            size_t t_argc_a = a->value.namespace_access.template_argc;
            Node* t_argv_a = a->value.namespace_access.template_argv;
            size_t t_argc_b = b->value.namespace_access.template_argc;
            Node* t_argv_b = b->value.namespace_access.template_argv;
            if(t_argc_a != t_argc_b) { return false; }
            for(size_t argi = 0; argi < t_argc_a; argi += 1) {
                if(!template_arg_eq(t_argv_a + argi, t_argv_b + argi)) {
                    return false;
                }
            }
            return true;
        case POINTER_TYPE_NODE:
            Node* ptr_type_a = a->value.pointer_type.to;
            Node* ptr_type_b = b->value.pointer_type.to;
            return template_arg_eq(ptr_type_a, ptr_type_b);
    }
    panic("UNHANDLED NODE TYPE FOR TEMPLATE ARG! HOW DID THIS PARSE?");
}


Symbol symbol_new(
    Namespace path, Node node, Namespace definined_in, size_t used_path_count,
    Namespace* used_paths
) {
    Symbol s;
    s.path = path;
    s.node = node;
    s.defined_in = definined_in;
    s.used_path_count = used_path_count;
    s.used_paths = used_paths;
    s.variants_bsize = 1;
    s.variants = (Node*) malloc(sizeof(Node) * s.variants_bsize);
    s.variant_count = 0;
    return s;
}

size_t symbol_targc(Symbol* s) {
    switch(s->node.type) {
        case FUNCTION_NODE:
            return s->node.value.function.template_argc;
        case RECORD_NODE:
            return s->node.value.record.template_argc;
        case EXTERNAL_FUNCTION_NODE:
            return 0;
    }
    panic("UNHANDLED SYMBOL TYPE???");
}

size_t symbol_find_variant(
    Symbol* s, size_t argc, Node* argv, SymbolTable* symbols, Arena* arena
) {
    for(size_t vari = 0; vari < s->variant_count; vari += 1) {
        Node* variant = s->variants + vari;
        size_t variant_t_argc;
        Node* variant_t_argv;
        switch(variant->type) {
            case FUNCTION_NODE:
                variant_t_argc = variant->value.function.template_argc;
                variant_t_argv = variant->value.function.template_argv;
                break;
            case RECORD_NODE:
                variant_t_argc = variant->value.record.template_argc;
                variant_t_argv = variant->value.record.template_argv;
                break;
            case EXTERNAL_FUNCTION_NODE:
                variant_t_argc = 0;
                break;
        }
        if(variant_t_argc != argc) {
            panic("Invalid template arg count!");
        }
        bool eq = true;
        for(size_t argi = 0; argi < argc; argi += 1) {
            if(!template_arg_eq(variant_t_argv + argi, argv + argi)) {
                eq = false;
                break;
            }
        }
        if(!eq) { continue; }
        return vari;
    }
    size_t symbol_t_argc;
    String* symbol_t_argnamev;
    switch(s->node.type) {
        case FUNCTION_NODE:
            symbol_t_argc = s->node.value.function.template_argc;
            symbol_t_argnamev = s->node.value.function.template_argnamev;
            break;
        case RECORD_NODE:
            symbol_t_argc = s->node.value.record.template_argc;
            symbol_t_argnamev = s->node.value.record.template_argnamev;
            break;
        case EXTERNAL_FUNCTION_NODE:
            symbol_t_argc = 0;
            break;
    }
    if(symbol_t_argc != argc) {
        panic("Invalid template arg count!");
    }
    if(s->variant_count + 1 > s->variants_bsize) {
        s->variants_bsize *= 2;
        s->variants = (Node*) realloc(
            s->variants, sizeof(Node) * s->variants_bsize
        );
    }
    size_t variant_idx = s->variant_count;
    Node* variant_node = s->variants + variant_idx;
    s->variant_count += 1;
    *variant_node = s->node;
    switch(variant_node->type) {
        case FUNCTION_NODE:
            variant_node->value.function.template_argv = argv;
            break;
        case RECORD_NODE:
            variant_node->value.record.template_argv = argv;
            break;
    }
    TemplateArgs targs = targs_new();
    for(size_t argi = 0; argi < argc; argi += 1) {
        targs_add(&targs, symbol_t_argnamev[argi], argv[argi]);
    }
    *variant_node = monomorphize_node(&s->node, s, symbols, arena, &targs);
    targs_free(&targs);
    return variant_idx;
}

Symbol* s_table_lookup(SymbolTable* table, Namespace path) {
    for(size_t symboli = 0; symboli < table->count; symboli += 1) {
        Namespace spath = table->symbols[symboli].path;
        if(spath.length != path.length) { continue; }
        bool eq = true;
        for(size_t elementi = 0; elementi < path.length; elementi += 1) {
            if(string_eq(spath.elements[elementi], path.elements[elementi])) {
                continue;
            }
            eq = false;
            break;
        }
        if(!eq) { continue; }
        return table->symbols + symboli;
    }
    return NULL;
}

void symbol_free(Symbol* s) {
    free(s->variants);
}


SymbolTable s_table_new() {
    SymbolTable table;
    table.count = 0;
    table.symbols_size = 32;
    table.symbols = (Symbol*) malloc(sizeof(Symbol) * table.symbols_size);
    return table;
}

void s_table_add(SymbolTable* table, Symbol symbol) {
    if(table->count >= table->symbols_size) {
        table->symbols_size *= 2;
        table->symbols = (Symbol*) realloc(
            table->symbols, sizeof(Symbol) * table->symbols_size
        );
    }
    table->symbols[table->count] = symbol;
    table->count += 1;
}

void s_table_free(SymbolTable* table) {
    for(size_t i = 0; i < table->count; i += 1) {
        symbol_free(&table->symbols[i]);
    }
    free(table->symbols);
}


DEF_ARRAY_BUILDER(String)
DEF_ARRAY_BUILDER(Namespace)

void collect_symbols(Block ast, SymbolTable* table, Arena* arena) {
    ArrayBuilder(Namespace) ub = arraybuilder_new(Namespace)();
    for(size_t nodei = 0; nodei < ast.length; nodei += 1) {
        Node n = ast.statements[nodei];
        if(n.type == USE_NODE) {
            arraybuilder_append(Namespace)(
                &ub, n.value.use.pathc, n.value.use.pathv
            );
        }
    }
    size_t used_path_count = ub.length;
    Namespace* used_paths = (Namespace*) arraybuilder_finish(Namespace)(
        &ub, arena
    );
    Namespace module;
    bool has_module;
    for(size_t nodei = 0; nodei < ast.length; nodei += 1) {
        Node n = ast.statements[nodei];
        switch(n.type) {
            case MODULE_NODE:
                module = n.value.module.path;
                has_module = true;
                break;
            case RECORD_NODE:
            case FUNCTION_NODE:
            case EXTERNAL_FUNCTION_NODE:
                if(!has_module) { panic("Missing module declaration!"); }
                ArrayBuilder(String) pb = arraybuilder_new(String)();
                arraybuilder_append(String)(
                    &pb, module.length, module.elements
                );
                Namespace* spath;
                switch(n.type) {
                    case RECORD_NODE: 
                        spath = &n.value.record.path;
                        break;
                    case FUNCTION_NODE:
                        spath = &n.value.function.path;
                        break;
                    case EXTERNAL_FUNCTION_NODE:
                        spath = &n.value.external_function.path;
                        break;
                }
                arraybuilder_append(String)(
                    &pb, spath->length, spath->elements
                );
                Namespace cpath = (Namespace) {
                    .length = pb.length,
                    .elements = (String*) arraybuilder_finish(String)(
                        &pb, arena
                    )
                };
                *spath = cpath;
                s_table_add(table, symbol_new(
                    cpath, n, module, used_path_count, used_paths
                ));
                break;
        }
    }
}