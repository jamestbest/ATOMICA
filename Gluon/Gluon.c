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
#include "shared/String.h"
#include "Gluon/File/File.h"

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
    logger_printf("<<PREJOB>> id: %zu file: %zu\n", job.job_id, job.file_id);

    const FileInfo info= FileInfo_arrayg_get(&file_infos, job.file_id);

    if (job.type == JOB_LEX) {
        set_token_cache(0, 0);
    }

    if (job.type > JOB_LEX) {
        set_lex_buffer(info.buffer);
    }

    if (job.type == JOB_PARSE) {
        set_token_cache(job.data.parse_source.start_line, job.data.parse_source.start_idx);
    }
}

static void post_job(const Job job) {
    logger_printf("<<POSTJOB>> id: %zu file: %zu\n", job.job_id, job.file_id);
    check_job_dependencies(job);
    switch (job.type) {
        case JOB_LEX: {
            const Job next_job= create_tls_job(job.result.lex_result, NO_DEPENDENCY, job.file_id);
            add_job(next_job);
            break;
        }
        case JOB_COLLECT_TLS: {
            logger_print("Finished collecting tls's.\n");
            break;
        }
        case JOB_PARSE: {
            const Job next_job= create_ir_job(job.result.parse_result, NO_DEPENDENCY, job.file_id);
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
            const int res= collect_tls(job->data.tls_source, job->file_id);
            if (res != EC_SUCCESS) job->state= JS_ERROR;
            break;
        }
        case JOB_PARSE: {
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

size_t create_file_info(const char* filename, const char* file_path, FileID from_file_id) {
    const FileID res= FileInfo_arrayg_add(&file_infos, (FileInfo) {
        .file_id= INVALID_FILE_ID,
        .file_name= filename,
        .file_path= file_path,
        .from_file_id= from_file_id,
        .jobs= JobSkeleton_arrayg_construct(25)
    });

    pthread_mutex_lock(&file_infos.mutex);
    (&file_infos.arr.data[res])->file_id= res;
    pthread_mutex_unlock(&file_infos.mutex);

    return res;
}

__thread Job current_job;
static void* worker_thread(void* arg) {
    logger_thread_init();

    while (true) {
        current_job= Job_qb_pop(&jobs);

        pre_job(current_job);

        run_job(&current_job);

        post_job(current_job);
    }
}

static void file_info_init() {
    file_infos= FileInfo_arrayg_construct(10);
    FileInfo_arrayg_add(&file_infos,
        (FileInfo) {
            .file_name= "INVALID FILE INFO",
            .file_id= INVALID_FILE_ID,
            .file_path= "INVALID FILE INFO",
            .from_file_id= INVALID_FILE_ID,
        }
    );
}

size_t compile_file(const char* file_path, const FileID from_file_id) {
    // todo: check if this file is already being compiled
    //   if so return the file_id that already exists
    const FileID file_id= create_file_info(
        get_filename(file_path),
        file_path,
        from_file_id
    );
    add_job(create_lex_job(file_path, file_id));

    return file_id;
}

#define MAX_THREADS 10
static pthread_t threads[MAX_THREADS];

// the orchestrator
int orchestrate(const char* source_file_path) {
    jobs_init();
    file_info_init();

    compile_file(source_file_path, INVALID_FILE_ID);

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