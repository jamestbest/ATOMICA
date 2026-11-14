//
// Created by jamescoward on 14/11/2026.
//

#include "Logger.h"

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

#include "shared/Buffer.h"

static pthread_mutex_t print_lock;
static __thread Buffer log_buffer;
static __thread pthread_t id;

void logger_thread_init() {
    log_buffer= buffer_construct(1024);
    id= pthread_self();
}

void logger_init() {
    const int res= pthread_mutex_init(&print_lock, NULL);
    assert(res == 0);
}

void logger_commit() {
    if (log_buffer.pos == 0) return;

    pthread_mutex_lock(&print_lock);
    printf("%s", (char*)log_buffer.data);
    buffer_clear(&log_buffer);
    pthread_mutex_unlock(&print_lock);
}

void logger_fprint(FILE* file, const char* message) {
    pthread_mutex_lock(&print_lock);
    fprintf(file, "%s\n", message);
    pthread_mutex_unlock(&print_lock);
}

void logger_print(const char* message) {
    pthread_mutex_lock(&print_lock);
    puts(message);
    pthread_mutex_unlock(&print_lock);
}

void logger_printf(const char* message, ...) {
    va_list args;
    va_start(args, message);
    pthread_mutex_lock(&print_lock);
    vprintf(message, args);
    pthread_mutex_unlock(&print_lock);
    va_end(args);
}

void logger_error(const char* error_message) {
    logger_fprint(stderr, error_message);
}

void logger_fadd(const char* format, ...) {
    va_list args;
    va_start(args, format);
    buffer_vfadd(&log_buffer, format, args);
    va_end(args);
}

void logger_vfadd(const char* format, va_list args) {
    buffer_vfadd(&log_buffer, format, args);
}

void logger_newline() {
    buffer_add(&log_buffer, "\n");
}
