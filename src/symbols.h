
#pragma once
#include "parser.h"


typedef struct Symbol Symbol;


typedef struct {
    size_t count;
    Symbol* symbols;
    size_t symbols_size;
} SymbolTable;

SymbolTable s_table_new();
void s_table_add(SymbolTable* table, Symbol symbol);
Symbol* s_table_lookup(SymbolTable* table, Namespace path);
void s_table_free(SymbolTable* table);


typedef struct Symbol {
    Namespace path;
    Node node;
    size_t variant_count;
    Node* variants;
    size_t variants_bsize;
} Symbol;

Symbol symbol_new(Namespace path, Node node);
size_t symbol_targc(Symbol* s);
size_t symbol_find_variant(
    Symbol* s, size_t argc, Node* argv, SymbolTable* symbols, Arena* arena
);
void symbol_free(Symbol* s);


void collect_symbols(Block ast, SymbolTable* table, Arena* arena);