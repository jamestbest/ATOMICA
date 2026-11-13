//
// Created by jamescoward on 10/11/2026.
//

#include "Job.h"

#include "shared/QueueBlocking.h"
#include "shared/ArrayG.h"

QB_ADD(Job, Job)
ARRAY_ADD(Job, Job)
ARRAYG_ADD(Job, Job)

static size_t next_job_id= 1;

Job create_job(const JobType type, const JobData data, const size_t dependency_id) {
    return (Job) {
        .type= type,
        .data= data,
        .dependency_id= dependency_id,
        .job_id= next_job_id++,
        .state= JS_NONE,
        .result= (JobResult){0}
    };
}

Job create_lex_job(const char* const source) {
    return create_job(JOB_LEX, (JobData){.lex_source= source}, NO_DEPENDENCY);
}

Job create_parse_job(const MyTokenSource source, const size_t dependency_id) {
    return create_job(JOB_PARSE, (JobData){.parse_source= source}, dependency_id);
}

Job create_ir_job(const NodeIndex source, const size_t dependency_id) {
    return create_job(JOB_IR_GEN, (JobData){.ir_gen_source= source}, dependency_id);
}

Job create_tls_job(const TokenArray source, const size_t dependency_id) {
    return create_job(JOB_COLLECT_TLS, (JobData){.tls_source= source}, dependency_id);
}
