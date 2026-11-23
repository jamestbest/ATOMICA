//
// Created by jamescoward on 10/11/2026.
//

#ifndef ATOMICA_JOB_H
#define ATOMICA_JOB_H

#include "Lexer/Token.h"
#include "Parser/Node.h"

#include "shared/QueueBlocking.h"
#include "shared/ArrayG.h"

typedef size_t FileID;

typedef enum {
    JOB_LEX,
    JOB_COLLECT_TLS,
    JOB_PARSE,
    JOB_IR_GEN,
    JOB_TIR_GEN,
} JobType;

typedef union {
    const char* lex_source;
    TokenArray tls_source;
    MyTokenSource parse_source;
    NodeIndex ir_gen_source;
} JobData;


typedef void* UNUSED;
typedef union {
    TokenArray lex_result;
    UNUSED tls_result;
    NodeIndex parse_result;
} JobResult;

typedef enum {
    JS_ERROR,
    JS_COMPLETE,
    JS_IN_PROGRESS,
    JS_WAITING_FOR_DEPENDENCY,
    JS_WAITING_IN_QUEUE,
    JS_NONE
} JobState;

typedef struct {
    FileID file_id;
    size_t job_id;
    size_t dependency_id;
    JobType type;
    JobData data;
    JobResult result;
    JobState state;
} Job;

typedef struct {
    size_t id;
    JobType type;
} JobSkeleton;

QB_PROTO(Job, Job)
ARRAY_PROTO(Job, Job)
ARRAYG_PROTO(Job, Job)
ARRAY_PROTO(JobSkeleton, JobSkeleton)
ARRAYG_PROTO(JobSkeleton, JobSkeleton)

#define NO_DEPENDENCY (0)

Job create_job(JobType type, JobData data, size_t dependency_id, FileID file_id);
Job create_lex_job(const char* source, FileID file_id);
Job create_parse_job(MyTokenSource source, size_t dependency_id, FileID file_id);
Job create_ir_job(NodeIndex source, size_t dependency_id, FileID file_id);
Job create_tls_job(TokenArray source, size_t dependency_id, FileID file_id);

#endif //ATOMICA_JOB_H
