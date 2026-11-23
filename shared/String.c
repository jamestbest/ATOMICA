//
// Created by jamescoward on 25/10/2026.
//

#include "String.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// returns a pointer back into the file_path where the filename is
const char* get_filename(const char* file_path) {
    if (*file_path == '\0') return file_path;
#ifdef _WIN32
    const char* res= strrchr(file_path, '\\');
#else
    const char* res= strrchr(file_path, '/');
#endif
    if (res == NULL) return file_path;
    return res + 1;
}

// NOT INPLACE
// ALLOCATES MEMORY
const char* replace_filename(const char* file_path, const char* new_filename) {
    if (file_path == NULL || new_filename == NULL) return NULL;
    if (*file_path == '\0' || *new_filename == '\0') return NULL;

    size_t new_filename_size= strlen(new_filename);
    size_t new_size= strlen(file_path) + new_filename_size + 1;
    char* res= malloc(new_size);

    //  /home/file.atm
    const char* old_filename= get_filename(file_path);
    const size_t base_size= old_filename - file_path;
    memcpy(res, file_path, base_size);
    memcpy(&res[base_size], new_filename, new_filename_size);
    res[base_size + new_filename_size]= '\0';

    return res;
}
