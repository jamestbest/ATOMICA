//
// Created by jamescoward on 23/10/2026.
//

#ifndef ATOMICA_ARRAY_H
#define ATOMICA_ARRAY_H

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    void* data;
    size_t max_elements; // max slot (element wise, not byte wise)
    size_t pos; // point to next empty slot
    size_t element_size;
} Array;

#define ARRAY_MIN 10

Array array_construct(size_t element_size, size_t number_of_elements);
Array array_create(size_t element_size);
bool array_resize(Array* arr, size_t new_max_number_of_elements);
size_t array_add(Array* arr, void* elem);
void* array_ptr(const Array* arr, size_t index);
size_t array_add_bulk(Array* arr, void* elems, size_t number_of_elements);
void array_destroy(Array* arr);
bool array_set(const Array* arr, void* elem, size_t index);

#define ARRAY_PROTO(type, name)         \
    typedef struct {                    \
        type* data;                     \
        size_t max_elements;            \
        size_t pos;                     \
        size_t element_size;            \
    } name##Array;                      \
    name##Array name##_array_construct(size_t number_of_elements);  \
    name##Array name##_array_create();  \
    size_t name##_array_add(name##Array* arr, type elem);           \
    type* name##_array_ptr(const name##Array* arr, size_t index);   \
    type name##_array_get(const name##Array* arr, size_t index);    \
    size_t name##_array_add_bulk(name##Array* arr, type* elems, size_t number_of_elements); \
    void name##_array_destroy(name##Array* arr); \
    bool name##_array_set(const name##Array* arr, const type elem, size_t index);\

#define ARRAY_ADD(type, name) \
    name##Array name##_array_construct(size_t number_of_elements) {\
        const size_t bytes= sizeof(type) * number_of_elements; \
        void* data= malloc(bytes); \
        if (!data) assert(false && "OOM in array construct"); \
\
        return (name##Array) { \
            .data= data, \
            .pos= 0, \
            .max_elements= number_of_elements, \
            .element_size= sizeof(type) \
        }; \
    }\
    name##Array name##_array_create() {\
        return name##_array_construct(ARRAY_MIN);\
    }\
    size_t name##_array_add(name##Array* arr, type elem) {\
        return array_add((Array*)arr, &elem);\
    }\
    type* name##_array_ptr(const name##Array* arr, size_t index) {\
        return array_ptr((Array*)arr, index);\
    }\
    type name##_array_get(const name##Array* arr, size_t index) {\
        return *(type*)array_ptr((Array*)arr, index);\
    }\
    size_t name##_array_add_bulk(name##Array* arr, type* elems, size_t number_of_elements) { \
        return array_add_bulk((Array*)arr, (void*)elems, number_of_elements); \
    } \
    void name##_array_destroy(name##Array* arr) { \
        return array_destroy((Array*)arr); \
    } \
    bool name##_array_set(const name##Array* arr, type elem, size_t index) {\
        return array_set((Array*)arr, &elem, index);\
    }\



#define ARRAY_PROTO_ADD(type, name) \
    ARRAY_PROTO(type, name) \
    ARRAY_ADD(type, name)

#endif //ATOMICA_ARRAY_H
