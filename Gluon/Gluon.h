//
// Created by jamescoward on 11/11/2026.
//

#ifndef ATOMICA_GLUON_H
#define ATOMICA_GLUON_H

#include "Jobs/Job.h"

int orchestrate(const char* source_file);

void add_job(Job job);
void add_dependent_job(Job job);

#endif //ATOMICA_GLUON_H
