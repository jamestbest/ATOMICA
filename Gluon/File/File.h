//
// Created by jamestbest on 11/18/26.
//

#ifndef ATOMICA_FILE_H
#define ATOMICA_FILE_H

#include <stddef.h>
#include "Gluon/Jobs/Job.h"
#include "shared/Buffer.h"

typedef size_t FileID;
#define INVALID_FILE_ID 0

typedef struct {
    FileID file_id;
    const char* file_name;
    const char* file_path;
    FileID from_file_id;

    Buffer buffer;          // file contents + inserted content (#insert)
    TokenArray tokens;      // lexed tokens
    NodeIndex root_node;    //

    JobSkeletonArrayG jobs;
} FileInfo;
ARRAY_PROTO(FileInfo, FileInfo)
ARRAYG_PROTO(FileInfo, FileInfo)

extern FileInfoArrayG file_infos;

#endif //ATOMICA_FILE_H
