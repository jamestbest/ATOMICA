//
// Created by jamescoward on 11/11/2026.
//

#include "Gluon.h"

#include <stdio.h>

#include "Error.h"
#include "Jobs/Job.h"
#include "Lexer/Lexer.h"
#include "Logger/Logger.h"
#include "Parser/Parser.h"
#include "Parser/TLSCollection/TLSPass.h"

// active todos
static JobQB jobs;
// jobs that require other jobs to complete
static JobArrayG dependent_jobs;

static void jobs_init() {
    jobs= Job_qb_construct(10);
    dependent_jobs= Job_arrayg_construct(10);
}

void add_job(Job job) {
    job.state= JS_WAITING_IN_QUEUE;
    Job_qb_add(&jobs, job);
}

void add_dependent_job(Job job) {
    job.state= JS_WAITING_FOR_DEPENDENCY;
    Job_arrayg_add(&dependent_jobs, job);
}

void check_job_dependencies(Job job) {
    pthread_mutex_lock(&dependent_jobs.mutex);
    for (int i = 0; i < dependent_jobs.arr.pos; ++i) {
        const Job* const j= Job_array_ptr(&dependent_jobs.arr, i);
        if (j->dependency_id == job.job_id) {
            // this job can be made active
            // const Job j_copy= dependent_jobs.arr._remove_unordered;
            // Job_qb_add(&jobs, j_copy);
        }
    }
    pthread_mutex_unlock(&dependent_jobs.mutex);
}

static void pre_job(const Job job) {

}

static void post_job(const Job job) {
    check_job_dependencies(job);
    switch (job.type) {
        case JOB_LEX: {
            const Job next_job= create_tls_job(job.result.lex_result, NO_DEPENDENCY);
            add_job(next_job);
            break;
        }
        case JOB_COLLECT_TLS: {
            logger_print("Finished collecting tls's.\n");
            break;
        }
        case JOB_PARSE: {
            const Job next_job= create_ir_job(job.result.parse_result, NO_DEPENDENCY);
            add_job(next_job);
            break;
        }
        case JOB_IR_GEN:
            break;
        case JOB_TIR_GEN:
            break;
    }
}

static void run_job(Job* job) {
    switch (job->type) {
        case JOB_LEX: {
            const LexRes res= lex(job->data.lex_source);
            if (res.errcode != EC_SUCCESS) job->state= JS_ERROR;
            job->result.lex_result= res.tokens;
            break;
        }
        case JOB_COLLECT_TLS: {
            const int res= collect_tls(job->data.tls_source);
            if (res != EC_SUCCESS) job->state= JS_ERROR;
            break;
        }
        case JOB_PARSE: {
            set_token_cache(job->data.parse_source.start_line, job->data.parse_source.start_idx);

            const NodeRes res= parse(job->data.parse_source);
            if (res.errcode != EC_SUCCESS) job->state= JS_ERROR;
            job->result.parse_result= res.index;

            break;
        }
        case JOB_IR_GEN:
            break;
        case JOB_TIR_GEN:
            break;
    }
}

static void* worker_thread(void* arg) {
    logger_thread_init();

    while (true) {
        Job job= Job_qb_pop(&jobs);

        pre_job(job);

        run_job(&job);

        post_job(job);
    }
}

#define MAX_THREADS 10
static pthread_t threads[MAX_THREADS];

// the orchestrator
int orchestrate(const char* source_file) {
    jobs_init();

    add_job(create_lex_job(source_file));

    for (size_t i = 0; i < MAX_THREADS; ++i) {
        const int res= pthread_create(&threads[i], NULL, worker_thread, NULL);
        assert(res == 0);
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        const int res= pthread_join(threads[i], NULL);
        assert(res == 0);
    }

    return EC_SUCCESS;
}