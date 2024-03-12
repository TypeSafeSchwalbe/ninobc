
#include "symbols.h"


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
    free(table->symbols);
}


DEF_ARRAY_BUILDER(String)

void collect_symbols(Block ast, SymbolTable* table, Arena* arena) {
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
                Namespace spath;
                switch(n.type) {
                    case RECORD_NODE: 
                        spath = n.value.record.path;
                        break;
                    case FUNCTION_NODE:
                        spath = n.value.function.path;
                        break;
                    case EXTERNAL_FUNCTION_NODE:
                        spath = n.value.external_function.path;
                        break;
                }
                arraybuilder_append(String)(
                    &pb, spath.length, spath.elements
                );
                Symbol symbol = (Symbol) {
                    .node = n,
                    .path = (Namespace) {
                        .length = pb.length,
                        .elements = (String*) arraybuilder_finish(String)(
                            &pb, arena
                        )
                    }
                };
                s_table_add(table, symbol);
                break;
        }
    }
}