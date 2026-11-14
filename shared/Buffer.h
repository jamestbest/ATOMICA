//
// Created by jamescoward on 14/11/2026.
//

#ifndef ATOMICA_BUFFER_H
#define ATOMICA_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t* data;
    size_t size;
    size_t pos;
} Buffer;

#define BUFF_MIN 64

int read_file_into_buffer(const char* file_path, Buffer* out);

Buffer buffer_construct(size_t bytes);
Buffer buffer_create();
void buffer_add(Buffer* buff, const char* string);
void buffer_fadd(Buffer* buff, const char* format, ...);
void buffer_vfadd(Buffer* buff, const char* format, va_list args);
bool buffer_resize(Buffer* buff, size_t new_size);
void buffer_clear(Buffer* buff);

#endif //ATOMICA_BUFFER_H
