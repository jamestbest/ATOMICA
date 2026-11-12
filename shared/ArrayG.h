//
// Created by jamescoward on 11/11/2026.
//

#ifndef ATOMICA_ARRAYG_H
#define ATOMICA_ARRAYG_H

#include <pthread.h>

#include "shared/Array.h"

typedef struct {
    Array arr;
    pthread_mutex_t mutex;
} ArrayG;

ArrayG  arrayg_construct(size_t element_size, size_t number_of_elements);
ArrayG  arrayg_create(size_t element_size);
bool    arrayg_resize(ArrayG* arr, size_t new_max_number_of_elements);
size_t  arrayg_add(ArrayG* arr, void* elem);
size_t  arrayg_add_dyn(ArrayG* arr, void* elem, size_t elem_size);
void*   arrayg_ptr(ArrayG* arr, size_t index);
size_t  arrayg_add_bulk(ArrayG* arr, void* elems, size_t number_of_elements);
void    arrayg_destroy(ArrayG* arr);
bool    arrayg_set(ArrayG* arr, void* elem, size_t index);
bool    arrayg_pop(ArrayG* arr, void* output);

// this requires ARRAY_PROTO of type, name to already be defined
#define ARRAYG_PROTO(type, name) \
    typedef struct {\
        name##Array arr;\
        pthread_mutex_t mutex;\
    } name##ArrayG;\
    name##ArrayG name##_arrayg_construct(size_t number_of_elements);  \
    name##ArrayG name##_arrayg_create();  \
    size_t name##_arrayg_add(name##ArrayG* arr, type elem);           \
    type* name##_arrayg_ptr(name##ArrayG* arr, size_t index);   \
    type name##_arrayg_get(name##ArrayG* arr, size_t index);    \
    size_t name##_arrayg_add_bulk(name##ArrayG* arr, type* elems, size_t number_of_elements); \
    void name##_arrayg_destroy(name##ArrayG* arr); \
    bool name##_arrayg_set(name##ArrayG* arr, type elem, size_t index);\
    type name##_arrayg_pop(name##ArrayG* arr);\

#define ARRAYG_ADD(type, name) \
    name##ArrayG name##_arrayg_construct(size_t number_of_elements) {\
        ArrayG res= arrayg_construct(sizeof(type), number_of_elements); \
        return *(name##ArrayG*)&res;\
    }\
    name##ArrayG name##_arrayg_create() {\
        return name##_arrayg_construct(ARRAY_MIN);\
    }\
    size_t name##_arrayg_add(name##ArrayG* arr, type elem) {\
        return arrayg_add((ArrayG*)arr, &elem);\
    }\
    type* name##_arrayg_ptr(name##ArrayG* arr, size_t index) {\
        return arrayg_ptr((ArrayG*)arr, index);\
    }\
    type name##_arrayg_get(name##ArrayG* arr, size_t index) {\
        return *(type*)arrayg_ptr((ArrayG*)arr, index);\
    }\
    size_t name##_arrayg_add_bulk(name##ArrayG* arr, type* elems, size_t number_of_elements) {\
        return arrayg_add_bulk((ArrayG*)arr, elems, number_of_elements);\
    }\
    void name##_arrayg_destroy(name##ArrayG* arr) {\
        arrayg_destroy((ArrayG*)arr);\
    }\
    bool name##_arrayg_set(name##ArrayG* arr, type elem, size_t index) {\
        return arrayg_set((ArrayG*)arr, &elem, index);\
    }\
    type name##_arrayg_pop(name##ArrayG* arr) {\
        type res;\
        assert(arrayg_pop((ArrayG*)arr, &res) == true);\
        return res;\
    }\


#endif //ATOMICA_ARRAYG_H
