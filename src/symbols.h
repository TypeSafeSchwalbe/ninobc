
#pragma once
#include "parser.h"


typedef struct {
    Namespace path;
    Node node;
} Symbol;

typedef struct {
    size_t count;
    Symbol* symbols;
    size_t symbols_size;
} SymbolTable;

SymbolTable s_table_new();
void s_table_add(SymbolTable* table, Symbol symbol);
void s_table_free(SymbolTable* table);


void collect_symbols(Block ast, SymbolTable* table, Arena* arena);