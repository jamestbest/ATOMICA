//
// Created by jamescoward on 10/11/2026.
//

#ifndef ATOMICA_QUEUEBLOCKING_H
#define ATOMICA_QUEUEBLOCKING_H

#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define sem_t HANDLE
#else
#include <semaphore.h>
#endif

typedef struct {
    void* data;
    size_t read_head;
    size_t write_head;
    size_t current_size;
    size_t max_size;
    size_t element_size;
    pthread_mutex_t mutex;
    sem_t sem;
} QueueB;

#define QUEUE_MIN 4

QueueB qb_construct(size_t max_elements, size_t element_size);
QueueB qb_create(size_t element_size);
bool qb_resize_no_lock(QueueB* q, size_t new_size);
void qb_add(QueueB* q, void* elem);
void qb_pop(QueueB* q, void* res);
void qb_destroy(QueueB* q);

#define QB_PROTO(type, name) \
    typedef struct {\
        type* data;\
        size_t read_head;\
        size_t write_head;\
        size_t current_size;\
        size_t max_size;\
        size_t element_size;\
        pthread_mutex_t mutex;\
        sem_t sem;\
    } name##QB; \
    name##QB name##_qb_construct(size_t max_elements);\
    name##QB name##_qb_create();\
    void name##_qb_add(name##QB* q, type elem);\
    type name##_qb_pop(name##QB* q);\
    void name##_qb_destroy(name##QB* q);

#define QB_ADD(type, name) \
    name##QB name##_qb_construct(size_t max_elements) { \
        QueueB q= qb_construct(max_elements, sizeof(type)); \
        return *(name##QB*)&q; \
    } \
    name##QB name##_qb_create() { \
        return name##_qb_construct(QUEUE_MIN); \
    } \
    void name##_qb_add(name##QB* q, type elem) { \
        return qb_add((QueueB*)q, &elem); \
    } \
    type name##_qb_pop(name##QB* q) { \
        type res;\
        qb_pop((QueueB*)q, &res);     \
        return res;\
    } \
    void name##_qb_destroy(name##QB* q) { \
        qb_destroy((QueueB*)q); \
    }

#endif //ATOMICA_QUEUEBLOCKING_H
