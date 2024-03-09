
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <string.h>


void panic(const char* reason);


typedef struct {
    char* buffer;
    size_t current_offset;
    size_t buffer_size;
} Arena;

Arena* arena_new();
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
