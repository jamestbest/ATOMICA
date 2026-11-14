//
// Created by jamescoward on 14/11/2026.
//

#include "Buffer.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Math.h"

int read_file_into_buffer(const char* file_path, Buffer* out) {
    FILE* file= fopen(file_path, "r");
    if (!file) return EXIT_FAILURE;

    assert(fseek(file, 0, SEEK_END) == 0);
    const long file_size= ftell(file);
    assert(fseek(file, 0, SEEK_SET) == 0);

    out->data= malloc(file_size + 1);
    if (!out->data) assert(false);

    const size_t bytes_read= fread(out->data, 1, file_size, file);
    out->data[bytes_read]= '\0';

    out->size= bytes_read;
    out->pos= bytes_read;

    return EXIT_SUCCESS;
}

Buffer buffer_construct(size_t bytes) {
    uint8_t* data= malloc(bytes);
    if (!data) assert(false && "Malloc failed in buffer construct");

    return (Buffer) {
        .pos= 0,
        .data= data,
        .size= bytes
    };
}

Buffer buffer_create() {
    return buffer_construct(BUFF_MIN);
}

void buffer_clear(Buffer* buff) {
    if (buff->size == 0) return;

    buff->data[0]= '\0';
    buff->pos= 0;
}

bool buffer_resize(Buffer* buff, size_t new_size) {
    uint8_t* new_data= realloc(buff->data, new_size);
    if (!new_data) return false;

    *buff= (Buffer) {
        .data= new_data,
        .pos= buff->pos,
        .size= new_size
    };

    return true;
}

void buffer_add(Buffer* buff, const char* string) {
    const size_t len= strlen(string);
    if (buff->pos + len >= buff->size) {
        const size_t new_size= MAX(buff->pos + len + 1, buff->size << 1);
        if (!buffer_resize(buff, new_size)) assert(false && "Failed to grow buffer");
    }

    // + 1 to include the null terminator
    memcpy(&buff->data[buff->pos], string, len + 1);
    buff->pos += len;
}

void buffer_vfadd(Buffer* buff, const char* format, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    const int bytes_needed= vsnprintf(NULL, 0, format, args) + 1;

    if (bytes_needed < 0) assert(false && "Failed to vslogger_printf in buffer_fadd");

    if (bytes_needed + buff->pos >= buff->size) {
        const size_t new_size= MAX(buff->pos + bytes_needed, buff->size << 1);
        if (!buffer_resize(buff, new_size)) assert(false && "Failed to resize in buffer fadd");
    }

    const int bytes_added= vsnprintf((char*)&buff->data[buff->pos], bytes_needed, format, args_copy);
    va_end(args_copy);

    // BYTES ADDED DOES NOT INCLUDE THE NULL TERMINATOR

    assert(bytes_added + 1 == bytes_needed);
    buff->pos += bytes_added;
}

void buffer_fadd(Buffer* buff, const char* format, ...) {
    va_list args;
    va_start(args, format);
    buffer_vfadd(buff, format, args);
    va_end(args);
}
