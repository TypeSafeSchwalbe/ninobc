
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


void panic(const char* reason) {
    puts(reason);
    exit(1);
}


Arena arena_new(size_t buffer_size) {
    Arena a;
    a.buffer_size = buffer_size;
    a.current_offset = 0;
    a.buffer = (char*) malloc(buffer_size + sizeof(Arena));
    a.next_buffer = NULL;
    return a;
}

void* arena_alloc(Arena* a, size_t n) {
    if(a->current_offset + n > a->buffer_size) {
        if(a->next_buffer == NULL) {
            size_t next_size = a->buffer_size * 2;
            while(n > next_size) { next_size *= 2; }
            Arena* next = (Arena*) (a->buffer + a->buffer_size);
            *next = arena_new(next_size);
            a->next_buffer = next;
        }
        return arena_alloc(a->next_buffer, n);
    }
    char* p = a->buffer + a->current_offset;
    a->current_offset += n;
    return (void*) p;
}

void arena_free(Arena* a) {
    Arena current = *a;
    while(true) {
        char* buffer = current.buffer;
        Arena* next_p = current.next_buffer;
        if(next_p != NULL) { current = *next_p; }
        free(buffer);
        if(next_p == NULL) { break; }
    }
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


