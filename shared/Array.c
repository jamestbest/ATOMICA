//
// Created by jamescoward on 23/10/2026.
//

#include "Array.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "shared/Math.h"

Array array_construct(size_t element_size, size_t number_of_elements) {
    const size_t bytes= element_size * number_of_elements;
    void* data= malloc(bytes);
    if (!data) assert(false && "OOM in array construct");

    return (Array) {
        .data= data,
        .pos= 0,
        .max_elements= number_of_elements,
        .element_size= element_size
    };
}

Array array_create(size_t element_size) {
    return array_construct(element_size, ARRAY_MIN);
}

bool array_resize(Array* arr, size_t new_max_number_of_elements) {
    void* const new_data= realloc(arr->data, new_max_number_of_elements * arr->element_size);
    if (!new_data) assert(false && "OOM in array_resize");

    arr->data= new_data;
    arr->max_elements= new_max_number_of_elements;

    return true;
}

size_t array_add(Array* arr, void* elem) {
    if (arr->pos >= arr->max_elements) {
        if (!array_resize(arr, arr->max_elements << 1)) assert(false && "Failed to resize array in array add");
    }

    memcpy(arr->data + (arr->pos * arr->element_size), elem, arr->element_size);

    return arr->pos++;
}

void* array_ptr(const Array* arr, const size_t index) {
    if (index >= arr->pos) return NULL;
    return arr->data + (index * arr->element_size);
}

size_t array_add_bulk(Array* arr, void* elems, size_t number_of_elements) {
    if (arr->pos + number_of_elements > arr->max_elements) {
        if (!array_resize(arr, MAX(arr->pos + number_of_elements, arr->max_elements << 1))) assert(false);
    }

    memcpy(arr->data + arr->pos * arr->element_size, elems, number_of_elements * arr->element_size);

    const size_t ret_idx= arr->pos + number_of_elements;
    arr->pos += number_of_elements;

    return ret_idx;
}

void array_destroy(Array* arr) {
    free(arr->data);
    *arr= (Array) {
        .data= NULL,
        .pos= 0,
        .max_elements= 0,
        .element_size= 0
    };
}

// array cannot grow, index must be in range
bool array_set(const Array* arr, void* elem, size_t index) {
    if (index >= arr->pos) return false;
    memcpy(arr->data + index * arr->element_size, elem, arr->element_size);
    return true;
}

