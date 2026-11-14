//
// Created by jamescoward on 10/11/2026.
//

#include "QueueBlocking.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "Math.h"

QueueB qb_construct(size_t max_elements, size_t element_size) {
    void* data= malloc(
        max_elements * element_size
    );
    if (!data) assert(false);

#ifdef _WIN32
    sem_t sem= CreateSemaphoreA(
        NULL,
        0,
        UINT32_MAX >> 1,
        NULL
    );

    if (sem == NULL) {
        assert(false);
    }
#else
    sem_t sem;
    const int sem_init_res= sem_init(&sem, 0, 0);
    assert(sem_init_res == 0);
#endif

    QueueB res= (QueueB) {
        .data= data,
        .read_head= 0,
        .write_head= 0,
        .current_size= 0,
        .element_size= element_size,
        .max_size= max_elements,
        .sem= sem
    };

    int mutex_res= pthread_mutex_init(&res.mutex, NULL);
    if (mutex_res != 0) assert(false);

    return res;
}

QueueB qb_create(size_t element_size) {
    return qb_construct(QUEUE_MIN, element_size);
}

bool qb_resize_no_lock(QueueB* q, const size_t new_max_elements) {
    // |---W   R-----|  -> |---W   R-----        | -> |R---------W              |
    // |R-------W    |
    assert(q->max_size < new_max_elements);
    void* new_data= malloc(new_max_elements * q->element_size);

    if (!new_data) return false;

    // elements between read head and write head (or end)
    const size_t right_count= q->write_head < q->read_head ?
        q->max_size - q->read_head :
        q->write_head;
    // elements between start and write head, or none if r<w
    const size_t left_count= q->write_head < q->read_head ?
        q->write_head : 0;

    memcpy(new_data, (uint8_t*)q->data + q->read_head * q->element_size, right_count * q->element_size);
    if (left_count != 0) {
        memcpy(new_data + (right_count * q->element_size), q->data, left_count * q->element_size);
    }

    q->data= new_data;
    q->max_size= new_max_elements;
    q->read_head= 0;
    q->write_head= right_count + left_count;

    return true;
}

void qb_add(QueueB* q, void* elem) {
    pthread_mutex_lock(&q->mutex);
    if (q->current_size >= q->max_size) {
        const size_t new_size= MAX(QUEUE_MIN, q->max_size << 1);
        if (!qb_resize_no_lock(q, new_size)) assert(false);
    }

    memcpy((uint8_t*)q->data + (q->write_head * q->element_size), elem, q->element_size);

    q->write_head= (q->write_head + 1) % (q->max_size + 1);
    q->current_size++;

    pthread_mutex_unlock(&q->mutex);

#ifdef _WIN32
    ReleaseSemaphore(q->sem, 1, NULL);
#else
    sem_post(&q->sem);
#endif
}

void qb_pop(QueueB* q, void* res) {
#ifdef _WIN32
    WaitForSingleObject(q->sem, INFINITE);
#else
    sem_wait(&q->sem);
#endif

    pthread_mutex_lock(&q->mutex);
    assert(q->current_size != 0);

    memcpy(res, (uint8_t*)q->data + (q->read_head * q->element_size), q->element_size);
    q->read_head= (q->read_head + 1) % (q->max_size + 1);
    q->current_size--;

    pthread_mutex_unlock(&q->mutex);
}

void qb_destroy(QueueB* q) {
    if (pthread_mutex_destroy(&q->mutex) != 0) assert(false);
#ifdef _WIN32
    if (CloseHandle(q->sem) != 0) assert(false);
#else
    sem_destroy(&q->sem);
#endif

    q->current_size= 0;
    q->read_head= 0;
    q->write_head= 0;
    q->max_size= 0;
}

