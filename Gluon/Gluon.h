//
// Created by jamescoward on 11/11/2026.
//

#ifndef ATOMICA_GLUON_H
#define ATOMICA_GLUON_H

#include "Jobs/Job.h"

int orchestrate(const char* source_file_path);

void add_job(Job job);
void add_dependent_job(Job job);

extern __thread Job current_job;
size_t compile_file(const char* file_path, FileID from_file_id);

#endif //ATOMICA_GLUON_H
