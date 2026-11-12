//
// Created by jamescoward on 11/11/2026.
//

#include "ArrayG.h"

ArrayG arrayg_construct(size_t element_size, size_t number_of_elements) {
    ArrayG res= (ArrayG) {
        .arr= array_construct(element_size, number_of_elements),
    };

    const int errcode= pthread_mutex_init(&res.mutex, NULL);
    assert(errcode == 0);

    return res;
}

ArrayG arrayg_create(size_t element_size) {
    return arrayg_construct(element_size, ARRAY_MIN);
}

bool arrayg_resize(ArrayG* arr, size_t new_max_number_of_elements) {
    pthread_mutex_lock(&arr->mutex);
    const bool res= array_resize(&arr->arr, new_max_number_of_elements);
    pthread_mutex_unlock(&arr->mutex);

    return res;
}

size_t arrayg_add(ArrayG* arr, void* elem) {
    pthread_mutex_lock(&arr->mutex);
    const size_t res= array_add(&arr->arr, elem);
    pthread_mutex_unlock(&arr->mutex);
    return res;
}

size_t arrayg_add_dyn(ArrayG* arr, void* elem, size_t elem_size) {
    pthread_mutex_lock(&arr->mutex);
    const size_t res= array_add_dyn(&arr->arr, elem, elem_size);
    pthread_mutex_unlock(&arr->mutex);
    return res;
}

size_t arrayg_add_bulk(ArrayG* arr, void* elems, size_t number_of_elements) {
    pthread_mutex_lock(&arr->mutex);
    const size_t res= array_add_bulk(&arr->arr, elems, number_of_elements);
    pthread_mutex_unlock(&arr->mutex);
    return res;
}

void* arrayg_ptr(ArrayG* arr, size_t index) {
    pthread_mutex_lock(&arr->mutex);
    void* const res= array_ptr(&arr->arr, index);
    pthread_mutex_unlock(&arr->mutex);

    return res;
}

void arrayg_destroy(ArrayG* arr) {
    pthread_mutex_lock(&arr->mutex);
    array_destroy(&arr->arr);
    pthread_mutex_unlock(&arr->mutex);
}

bool arrayg_set(ArrayG* arr, void* elem, size_t index) {
    pthread_mutex_lock(&arr->mutex);
    const bool res= array_set(&arr->arr, elem, index);
    pthread_mutex_unlock(&arr->mutex);
    return res;
}

bool arrayg_pop(ArrayG* arr, void* output) {
    pthread_mutex_lock(&arr->mutex);
    const bool res= array_pop(&arr->arr, output);
    pthread_mutex_unlock(&arr->mutex);
    return res;
}

