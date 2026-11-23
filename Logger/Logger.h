//
// Created by jamescoward on 14/11/2026.
//

#ifndef ATOMICA_LOGGER_H
#define ATOMICA_LOGGER_H

#include <stdarg.h>
#include <stdio.h>

void logger_init();
void logger_thread_init();
void logger_commit();

void logger_fadd(const char* format, ...);
void logger_vfadd(const char* format, va_list args);
void logger_newline();

void logger_error(const char* error_message);
void logger_errorf(const char* error_message, ...);
void logger_fprint(FILE* file, const char* message);
void logger_print(const char* message);
void logger_printf(const char* message, ...);

#endif //ATOMICA_LOGGER_H
