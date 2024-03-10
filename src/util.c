
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


void panic(const char* reason) {
    puts(reason);
    exit(1);
}


Arena arena_new() {
    Arena a;
    a.buffer_size = 1028;
    a.current_offset = 0;
    a.buffer = (char*) malloc(a.buffer_size);
    return a;
}

void* arena_alloc(Arena* a, size_t n) {
    size_t new_offset = a->current_offset + n;
    size_t new_buffer_size = a->buffer_size;
    while(new_offset > new_buffer_size) {
        new_buffer_size *= 2;
    }
    if(new_buffer_size > a->buffer_size) {
        a->buffer_size = new_buffer_size;
        a->buffer = (char*) realloc(a->buffer, a->buffer_size);
    }
    char* p = a->buffer + a->current_offset;
    a->current_offset = new_offset;
    return (void*) p;
}

void arena_free(Arena* a) {
    free(a->buffer);
}


String string_wrap_nt(const char* data) {
    return (String) {
        .data = data,
        .length = strlen(data)
    };
}

String string_wrap_nt_slice(const char* data, size_t length) {
    return (String) {
        .data = data,
        .length = length
    };
}

char string_char_at(String src, size_t offset) {
    return src.data[offset];
}

String string_slice(String src, size_t offset, size_t end) {
    return (String) {
        .data = src.data + offset,
        .length = end - offset
    };
}

bool string_starts_with(String src, String prefix) {
    return memcmp(src.data, prefix.data, prefix.length) == 0;
}


