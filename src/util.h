
#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


void panic(const char* reason);


typedef struct Arena Arena;

typedef struct Arena {
    char* buffer;
    size_t buffer_size;
    size_t current_offset;
    Arena* next_buffer;
} Arena;

Arena arena_new(size_t buffer_size);
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
bool string_eq(String a, String b);

#define STRING_AS_NT(src, var_name) \
    char var_name[(src).length + 1]; \
    memcpy(var_name, (src).data, (src).length); \
    var_name[(src).length] = '\0';


#define ArrayBuilder(t) ArrayBuilder_##t
#define arraybuilder_new(t) arraybuilder_new_##t
#define arraybuilder_append(t) arraybuilder_append_##t
#define arraybuilder_push(t) arraybuilder_push_##t
#define arraybuilder_finish(t) arraybuilder_finish_##t
#define arraybuilder_discard(t) arraybuilder_discard_##t
#define DEF_ARRAY_BUILDER(t) \
    \
    typedef struct { \
        t* buffer; \
        size_t length; \
        size_t buffer_size; \
    } ArrayBuilder(t); \
    \
    static ArrayBuilder(t) arraybuilder_new(t)() { \
        ArrayBuilder(t) builder; \
        builder.buffer_size = 16; \
        builder.length = 0; \
        builder.buffer = (t*) malloc(sizeof(t) * builder.buffer_size); \
        return builder; \
    } \
    \
    static void arraybuilder_append(t)(ArrayBuilder(t)* b, size_t valuec, t* valuev) { \
        size_t new_length = b->length + valuec; \
        size_t new_buffer_size = b->buffer_size; \
        while(new_length > new_buffer_size) { \
            new_buffer_size *= 2; \
        } \
        if(new_buffer_size > b->buffer_size) { \
            b->buffer_size = new_buffer_size; \
            b->buffer = (t*) realloc(b->buffer, sizeof(t) * b->buffer_size); \
        } \
        memcpy(b->buffer + b->length, valuev, sizeof(t) * valuec); \
        b->length = new_length; \
    } \
    \
    static void arraybuilder_push(t)(ArrayBuilder(t)* b, t value) { \
        arraybuilder_append(t)(b, 1, &value); \
    } \
    \
    static void* arraybuilder_finish(t)(ArrayBuilder(t)* b, Arena* a) { \
        void* p = arena_alloc(a, sizeof(t) * b->length); \
        memcpy(p, b->buffer, sizeof(t) * b->length); \
        free(b->buffer); \
        return p; \
    } \
    \
    static void arraybuilder_discard(t)(ArrayBuilder(t)* b) { \
        free(b->buffer); \
    }


typedef struct {
    size_t length;
    char* buffer;
    size_t buffer_size;
} StringBuilder;

StringBuilder stringbuilder_new();
void stringbuilder_push(StringBuilder* b, size_t c, const char* d);
void stringbuilder_push_nt_string(StringBuilder* b, const char* s);
void stringbuilder_push_string(StringBuilder* b, String s);
void stringbuilder_push_char(StringBuilder* b, char c);
void stringbuilder_free(StringBuilder* b);