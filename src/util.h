
#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


void panic(const char* reason);


typedef struct {
    char* buffer;
    size_t current_offset;
    size_t buffer_size;
} Arena;

Arena arena_new();
void* arena_alloc(Arena* a, size_t n);
void arena_free(Arena* a);


typedef struct {
    const char* data;
    size_t length;
} String;

String string_wrap_nt(const char* data);
String string_wrap_nt_slice(const char* data, size_t length);
char string_char_at(String src, size_t offset);
String string_slice(String src, size_t offset, size_t end);
bool string_starts_with(String src, String prefix);

#define STRING_AS_NT(src, var_name) \
    char var_name[(src).length + 1]; \
    memcpy(var_name, (src).data, (src).length); \
    var_name[(src).length] = '\0';


#define ArrayBuilder(t) ArrayBuilder_##t
#define arraybuilder_new(t) arraybuilder_new_##t
#define arraybuilder_push(t) arraybuilder_push_##t
#define arraybuilder_finish(t) arraybuilder_finish_##t
#define DEF_ARRAY_BUILDER(t) \
    \
    typedef struct { \
        t* buffer; \
        size_t length; \
        size_t buffer_size; \
    } ArrayBuilder(t); \
    \
    ArrayBuilder(t) arraybuilder_new(t)() { \
        ArrayBuilder(t) builder; \
        builder.buffer_size = 16; \
        builder.length = 0; \
        builder.buffer = (t*) malloc(sizeof(t) * builder.buffer_size); \
        return builder; \
    } \
    \
    void arraybuilder_push(t)(ArrayBuilder(t)* b, t value) { \
        size_t new_length = b->length + 1; \
        if(new_length > b->buffer_size) { \
            b->buffer_size *= 2; \
            b->buffer = (t*) realloc(b->buffer, sizeof(t) * b->buffer_size); \
        } \
        memcpy(b->buffer + b->length, &value, sizeof(t)); \
        b->length = new_length; \
    } \
    \
    void* arraybuilder_finish(t)(ArrayBuilder(t)* b, Arena* a) { \
        void* p = arena_alloc(a, sizeof(t) * b->length); \
        memcpy(p, b->buffer, sizeof(t) * b->length); \
        free(b->buffer); \
        return p; \
    }
