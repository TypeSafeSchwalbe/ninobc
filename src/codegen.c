
#include <stdio.h>
#include "codegen.h"

#define WRITE(s) stringbuilder_push_nt_string(out, s)
#define WRITE_S(s) stringbuilder_push_string(out, s)
#define WRITE_C(c) stringbuilder_push_char(out, c)

typedef struct RecordEntry {
    Namespace path;
    size_t variant;
} RecordEntry;

DEF_ARRAY_BUILDER(RecordEntry)

static void emit_path_element(String* element, StringBuilder* out) {
    for(size_t i = 0; i < element->length; i += 1) {
        char c = string_char_at(*element, i);
        if(c == '_') { WRITE("__"); }
        else { WRITE_C(c); }
    }
}

static void emit_path(Namespace* path, size_t variant, StringBuilder* out) {
    for(size_t i = 0; i < path->length; i += 1) {
        if(i > 0) { WRITE_C('_'); }
        emit_path_element(path->elements + i, out);
    }
    WRITE_C('_');
    size_t variant_str_len = snprintf(NULL, 0, "%zu", variant);
    char variant_str[variant_str_len];
    sprintf(variant_str, "%zu", variant);
    stringbuilder_push(out, variant_str_len, variant_str);
}

#define WRITE_TYPE(t) \
    emit_type(t, symbols, typesdefs, types, out)

static void emit_type(
    Node* n, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
);

static bool namespace_eq(Namespace a, Namespace b) {
    if(a.length != b.length) { return false; }
    for(size_t elementi = 0; elementi < a.length; elementi += 1) {
        if(!string_eq(a.elements[elementi], b.elements[elementi])) {
            return false;
        }
    }
    return true;
}

static void declare_type(
    Namespace path, size_t variant, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types
) {
    for(size_t typei = 0; typei < types->length; typei += 1) {
        RecordEntry* entry = ((RecordEntry*) types->buffer) + typei;
        if(namespace_eq(path, entry->path) && variant == entry->variant) {
            return;
        }
    }
    StringBuilder decl = stringbuilder_new();
    StringBuilder* out = &decl;
    Symbol* s = s_table_lookup(symbols, path);
    if(s == NULL) { return; }
    if(variant >= s->variant_count) { return; }
    Node* symbol = s->variants + variant;
    arraybuilder_push(RecordEntry)(types, (RecordEntry) {
        .path = path, .variant = variant
    });
    switch(symbol->type) {
        case RECORD_NODE:
            WRITE("typedef struct ");
            emit_path(&symbol->value.record.path, variant, out);
            WRITE(" { ");
            for(size_t argi = 0; argi < symbol->value.record.argc; argi += 1) {
                WRITE_TYPE(symbol->value.record.argtypev + argi);
                WRITE_C(' ');
                WRITE_S(symbol->value.record.argnamev[argi]);
                WRITE("; ");
            }
            WRITE("} ");
            emit_path(&symbol->value.record.path, variant, out);
            WRITE(";\n");
            break;
    }
    stringbuilder_push(typesdefs, decl.length, decl.buffer);
    stringbuilder_free(&decl);
}

static void emit_type(
    Node* n, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
) {
    switch(n->type) {
        case NAMESPACE_ACCESS_NODE:
            if(n->value.namespace_access.path.length == 1) {
                #define EMIT_CORE_TYPE(r, e) if(string_eq( \
                        n->value.namespace_access.path.elements[0], \
                        string_wrap_nt(r) \
                    )) { \
                        WRITE(e); \
                        return; \
                    }
                EMIT_CORE_TYPE("u8", "uint8_t");
                EMIT_CORE_TYPE("u16", "uint16_t");
                EMIT_CORE_TYPE("u32", "uint32_t");
                EMIT_CORE_TYPE("u64", "uint64_t");
                EMIT_CORE_TYPE("s8", "int8_t");
                EMIT_CORE_TYPE("s16", "int16_t");
                EMIT_CORE_TYPE("s32", "int32_t");
                EMIT_CORE_TYPE("s64", "int64_t");
                EMIT_CORE_TYPE("f32", "float");
                EMIT_CORE_TYPE("f64", "double");
                EMIT_CORE_TYPE("usize", "size_t");
                EMIT_CORE_TYPE("ssize", "ptrdiff_t");
                EMIT_CORE_TYPE("unit", "void");
                EMIT_CORE_TYPE("bool", "bool");
            }
            declare_type(
                n->value.namespace_access.path,
                n->value.namespace_access.variant,
                symbols, typesdefs, types
            );
            emit_path(
                &n->value.namespace_access.path,
                n->value.namespace_access.variant,
                out
            );
            return;
        case POINTER_TYPE_NODE:
            WRITE_TYPE(n->value.pointer_type.to);
            WRITE_C('*');
            return;
        default:
            panic("UNHANDLED TYPE NODE!");
    }
}

static void emit_block(
    Block block, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
);

#define WRITE_NODE(n) { \
    WRITE_C('('); \
    emit_node(n, symbols, typesdefs, types, out); \
    WRITE_C(')'); \
}

static void emit_node(
    Node* node, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
) {
    switch(node->type) {
        // case UNIT_LITERAL_NODE:
        // case MODULE_NODE:
        // case USE_NODE:
        // case FUNCTION_NODE:
        // case EXTERNAL_FUNCTION_NODE:
        // case RECORD_NODE:
        // case POINTER_TYPE_NODE:
        case INTEGER_LITERAL_NODE:
            WRITE_S(node->value.integer_literal.value);
            break;
        case FLOAT_LITERAL_NODE:
            WRITE_S(node->value.float_literal.value);
            break;
        case STRING_LITERAL_NODE:
            WRITE_S(node->value.string_literal.value);
            break;
        case BOOLEAN_LITERAL_NODE:
            WRITE_S(node->value.boolean_literal.value);
            break;
        case VARIABLE_NODE:
            WRITE_S(node->value.variable.name);
            break;
        case VARIABLE_DECLARATION_NODE:
            WRITE_TYPE(node->value.variable_declaration.type);
            WRITE_C(' ');
            WRITE_S(node->value.variable_declaration.name);
            WRITE(" = ");
            WRITE_NODE(node->value.variable_declaration.value);
            break;
        case ASSIGNMENT_NODE:
            WRITE_NODE(node->value.assignment.to);
            WRITE(" = ");
            WRITE_NODE(node->value.assignment.value);
            break;
        case ADDITION_NODE:
            WRITE_NODE(node->value.addition.a);
            WRITE(" + ");
            WRITE_NODE(node->value.addition.b);
            break;
        case SUBTRACTION_NODE:
            WRITE_NODE(node->value.subtraction.a);
            WRITE(" - ");
            WRITE_NODE(node->value.subtraction.b);
            break;
        case MULTIPLICATION_NODE:
            WRITE_NODE(node->value.multiplication.a);
            WRITE(" * ");
            WRITE_NODE(node->value.multiplication.b);
            break;
        case DIVISION_NODE:
            WRITE_NODE(node->value.division.a);
            WRITE(" / ");
            WRITE_NODE(node->value.division.b);
            break;
        case REMAINDER_NODE:
            WRITE_NODE(node->value.remainder.a);
            WRITE(" % ");
            WRITE_NODE(node->value.remainder.b);
            break;
        case NEGATION_NODE:
            WRITE_C('-');
            WRITE_NODE(node->value.negation.x);
            break;
        case BITWISE_AND_NODE:
            WRITE_NODE(node->value.bitwise_and.a);
            WRITE(" & ");
            WRITE_NODE(node->value.bitwise_and.b);
            break;
        case BITWISE_OR_NODE:
            WRITE_NODE(node->value.bitwise_or.a);
            WRITE(" | ");
            WRITE_NODE(node->value.bitwise_or.b);
            break;
        case BITWISE_XOR_NODE:
            WRITE_NODE(node->value.bitwise_xor.a);
            WRITE(" ^ ");
            WRITE_NODE(node->value.bitwise_xor.b);
            break;
        case LEFT_SHIFT_NODE:
            WRITE_NODE(node->value.left_shift.x);
            WRITE(" << ");
            WRITE_NODE(node->value.left_shift.n);
            break;
        case RIGHT_SHIFT_NODE:
            WRITE_NODE(node->value.right_shift.x);
            WRITE(" >> ");
            WRITE_NODE(node->value.right_shift.n);
            break;
        case BITWISE_NOT_NODE:
            WRITE_C('~');
            WRITE_NODE(node->value.bitwise_not.x);
            break;
        case LOGICAL_AND_NODE:
            WRITE_NODE(node->value.logical_and.a);
            WRITE(" && ");
            WRITE_NODE(node->value.logical_and.b);
            break;
        case LOGICAL_OR_NODE:
            WRITE_NODE(node->value.logical_or.a);
            WRITE(" || ");
            WRITE_NODE(node->value.logical_or.b);
            break;
        case LOGICAL_NOT_NODE:
            WRITE_C('!');
            WRITE_NODE(node->value.logical_not.x);
            break;
        case EQUALS_NODE:
            WRITE_NODE(node->value.equals.a);
            WRITE(" == ");
            WRITE_NODE(node->value.equals.b);
            break;
        case NOT_EQUALS_NODE:
            WRITE_NODE(node->value.not_equals.a);
            WRITE(" != ");
            WRITE_NODE(node->value.not_equals.b);
            break;
        case LESS_THAN_NODE:
            WRITE_NODE(node->value.less_than.a);
            WRITE(" < ");
            WRITE_NODE(node->value.less_than.b);
            break;
        case GREATER_THAN_NODE:
            WRITE_NODE(node->value.greater_than.a);
            WRITE(" > ");
            WRITE_NODE(node->value.greater_than.b);
            break;
        case LESS_THAN_EQUAL_NODE:
            WRITE_NODE(node->value.less_than_equal.a);
            WRITE(" < ");
            WRITE_NODE(node->value.less_than_equal.b);
            break;
        case GREATER_THAN_EQUAL_NODE:
            WRITE_NODE(node->value.greater_than_equal.a);
            WRITE(" > ");
            WRITE_NODE(node->value.greater_than_equal.b);
            break;
        case DEREF_NODE:
            WRITE_C('*');
            WRITE_NODE(node->value.deref.x);
            break;
        case ADDRESS_OF_NODE:
            WRITE_C('&');
            WRITE_NODE(node->value.address_of.x);
            break;
        case SIZE_OF_NODE:
            WRITE("sizeof(");
            WRITE_TYPE(node->value.size_of.t);
            WRITE_C(')');
            break;
        case MEMBER_ACCESS_NODE:
            WRITE_NODE(node->value.member_access.x);
            WRITE_C('.');
            WRITE_S(node->value.member_access.name);
            break;
        case NAMESPACE_ACCESS_NODE:
            emit_path(
                &node->value.namespace_access.path,
                node->value.namespace_access.variant,
                out
            );
            Symbol* accessed = s_table_lookup(
                symbols, node->value.namespace_access.path
            );
            if(accessed != NULL && (
                accessed->node.type == EXTERNAL_FUNCTION_NODE
                    || accessed->node.type == FUNCTION_NODE
            )) {
                WRITE("()");
            }
            break;
        case TYPE_CONVERSION_NODE:
            WRITE_C('(');
            WRITE_TYPE(node->value.type_conversion.to);
            WRITE(") ");
            WRITE_NODE(node->value.type_conversion.x);
            break;
        case RETURN_VALUE_NODE:
            WRITE("return");
            if(node->value.return_value.has_value) {
                WRITE_C(' ');
                WRITE_NODE(node->value.return_value.value);
            }
            break;
        case IF_ELSE_NODE:
            WRITE("if(");
            WRITE_NODE(node->value.if_else.condition);
            WRITE(") { ");
            emit_block(
                node->value.if_else.if_body, symbols, typesdefs, types, out
            );
            WRITE(" } else { ");
            emit_block(
                node->value.if_else.else_body, symbols, typesdefs, types, out
            );
            WRITE(" }");
            break;
        case WHILE_DO_NODE:
            WRITE("while(");
            WRITE_NODE(node->value.while_do.condition);
            WRITE(") { ");
            emit_block(
                node->value.while_do.body, symbols, typesdefs, types, out
            );
            WRITE(" }");
            break;
        case CALL_NODE:
            #define WRITE_ARGS() \
                WRITE_C('('); \
                for(size_t argi = 0; argi < node->value.call.argc; argi += 1) { \
                    if(argi > 0) { WRITE(", "); } \
                    WRITE_NODE(node->value.call.argv + argi); \
                } \
                WRITE_C(')');
            if(node->value.call.called->type == NAMESPACE_ACCESS_NODE) {
                Namespace called_path = node->value.call.called
                    ->value.namespace_access.path;
                size_t called_variant = node->value.call.called
                    ->value.namespace_access.variant;
                Symbol* called = s_table_lookup(symbols, called_path);
                if(called == NULL) {
                    WRITE("/* COULD NOT BE FOUND: '");
                    emit_path(&called_path, called_variant, out);
                    WRITE("' */");
                    return;
                }
                switch(called->node.type) {
                    case FUNCTION_NODE:
                        emit_path(&called_path, called_variant, out);
                        WRITE_ARGS();
                        break;
                    case EXTERNAL_FUNCTION_NODE:
                        WRITE_S(
                            called->node.value.external_function.external_name
                        );
                        WRITE_ARGS();
                        break;
                    case RECORD_NODE:
                        WRITE_C('(');
                        emit_path(&called_path, called_variant, out);
                        WRITE(") { ");
                        size_t memberc = called->node.value.record.argc;
                        String* membernamev = called->node.value.record.argnamev;
                        for(size_t argi = 0; argi < memberc; argi += 1) {
                            if(argi > 0) { WRITE(", "); }
                            WRITE_C('.');
                            WRITE_S(membernamev[argi]);
                            WRITE(" = ");
                            WRITE_NODE(node->value.call.argv + argi);
                        }
                        WRITE(" }");
                        break;
                }
            } else {
                WRITE_NODE(node->value.call.called);
                WRITE_ARGS();
            }
            break;
    }
}

static void emit_block(
    Block block, SymbolTable* symbols, 
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
) {
    for(size_t si = 0; si < block.length; si += 1) {
        if(si > 0) { WRITE_C(' '); }
        emit_node(block.statements + si, symbols, typesdefs, types, out);
        WRITE_C(';');
    }
}

static void emit_symbol_variant_pre(
    Node* symbol, size_t variant, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
) {
    switch(symbol->type) {
        case RECORD_NODE:
            WRITE("typedef struct ");
            emit_path(&symbol->value.record.path, variant, out);
            WRITE_C(' ');
            emit_path(&symbol->value.record.path, variant, out);
            WRITE(";\n");
            break;
        case FUNCTION_NODE:
            WRITE_TYPE(symbol->value.function.return_type);
            WRITE_C(' ');
            emit_path(&symbol->value.function.path, variant, out);
            WRITE_C('(');
            size_t fun_argc = symbol->value.function.argc;
            for(size_t argi = 0; argi < fun_argc; argi += 1) {
                if(argi > 0) { WRITE(", "); }
                WRITE_TYPE(symbol->value.function.argtypev + argi);
                WRITE_C(' ');
                WRITE_S(symbol->value.function.argnamev[argi]);
            }
            WRITE(");\n");
            break;
        case EXTERNAL_FUNCTION_NODE:
            WRITE("extern ");
            WRITE_TYPE(symbol->value.external_function.return_type);
            WRITE_C(' ');
            WRITE_S(symbol->value.external_function.external_name);
            WRITE_C('(');
            size_t ext_fun_argc = symbol->value.external_function.argc;
            for(size_t argi = 0; argi < ext_fun_argc; argi += 1) {
                if(argi > 0) { WRITE(", "); }
                WRITE_TYPE(symbol->value.external_function.argtypev + argi);
                WRITE_C(' ');
                WRITE_S(symbol->value.external_function.argnamev[argi]);
            }
            WRITE(");\n");
            break;
        default:
            panic("UNHANDLED NODE TYPE!");
    }
}

static void emit_symbol_variant(
    Node* symbol, size_t variant, SymbolTable* symbols,
    StringBuilder* typesdefs, ArrayBuilder(RecordEntry)* types,
    StringBuilder* out
) {
    switch(symbol->type) {
        case RECORD_NODE:
            // fully declared when used
            break;
        case FUNCTION_NODE:
            WRITE_TYPE(symbol->value.function.return_type);
            WRITE_C(' ');
            emit_path(&symbol->value.function.path, variant, out);
            WRITE_C('(');
            size_t fun_argc = symbol->value.function.argc;
            for(size_t argi = 0; argi < fun_argc; argi += 1) {
                if(argi > 0) { WRITE(", "); }
                WRITE_TYPE(symbol->value.function.argtypev + argi);
                WRITE_C(' ');
                WRITE_S(symbol->value.function.argnamev[argi]);
            }
            WRITE(") { ");
            emit_block(
                symbol->value.function.body, symbols, typesdefs, types, out
            );
            WRITE(" }\n");
            break;
        case EXTERNAL_FUNCTION_NODE:
            break;
        default:
            panic("UNHANDLED NODE TYPE!");
    }
}

StringBuilder generate_code(SymbolTable* symbols, Namespace* main) {
    StringBuilder out = stringbuilder_new();
    StringBuilder typesdefs = stringbuilder_new();
    ArrayBuilder(RecordEntry) types = arraybuilder_new(RecordEntry)();
    stringbuilder_push_nt_string(&out,
        "\n"
        "// C output generated by the Nino bootstrap compiler\n"
        "// https://github.com/typesafeschwalbe/ninobc\n"
        "\n"
        "#include <stddef.h>\n"
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "\n"
    );
    for(size_t symboli = 0; symboli < symbols->count; symboli += 1) {
        Symbol* symbol = symbols->symbols + symboli;
        for(size_t vari = 0; vari < symbol->variant_count; vari += 1) {
            emit_symbol_variant_pre(
                symbol->variants + vari, vari, symbols,
                &typesdefs, &types, &out
            );
        }
    }
    stringbuilder_push_nt_string(&out, "\n");
    stringbuilder_push(&out, typesdefs.length, typesdefs.buffer);
    stringbuilder_push_nt_string(&out, "\n");
    for(size_t symboli = 0; symboli < symbols->count; symboli += 1) {
        Symbol* symbol = symbols->symbols + symboli;
        for(size_t vari = 0; vari < symbol->variant_count; vari += 1) {
            emit_symbol_variant(
                symbol->variants + vari, vari, symbols,
                &typesdefs, &types, &out
            );
        }
    }
    if(main != NULL) {
        stringbuilder_push_nt_string(&out, "\n");
        stringbuilder_push_nt_string(&out, "int main() {\n");
        stringbuilder_push_nt_string(&out, "    ");
        emit_path(main, 0, &out);
        stringbuilder_push_nt_string(&out, "();\n");
        stringbuilder_push_nt_string(&out, "    return 0;\n");
        stringbuilder_push_nt_string(&out, "}\n");
    }
    stringbuilder_free(&typesdefs);
    arraybuilder_discard(RecordEntry)(&types);
    return out;
}