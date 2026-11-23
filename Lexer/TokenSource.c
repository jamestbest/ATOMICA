//
// Created by jamestbest on 11/19/26.
//

#include "TokenSource.h"

#include <string.h>

#include "shared/Math.h"
#include "Lexer/Token.h"

MyTokenSource clone_source(MyTokenSource source, size_t to, size_t from, size_t start_line) {
    Token* data= malloc((to - from + 1) * sizeof(Token));
    if (!data) assert(false)    ;

    memcpy(data, &source.data[from], (to - from + 1) * sizeof(Token));

    return (MyTokenSource) {
        .start_idx= 0,
        .current_idx= 0,
        .end_idx= to - from,
        .data= data,
        .start_line= start_line,
        .max_tokens= to - from + 1
    };
}

void resize_source(MyTokenSource* source, size_t new_size) {
    Token* new_data= realloc(source->data, new_size * sizeof(Token));
    if (!new_data) assert(false);

    source->max_tokens= new_size;
    source->data= new_data;
}

// insert additions.data at source.data[source_insert_index]
void combine_source(
    MyTokenSource* source,
    MyTokenSource addition,
    size_t source_insert_idx,
    size_t overwrite_amount
) {
    size_t addition_tokens= addition.end_idx - addition.current_idx + 1;

    assert(source->data != addition.data);

    const size_t overwrite_possible= MIN(addition_tokens, overwrite_amount);
    memcpy(&source->data[source_insert_idx], addition.data, overwrite_possible * sizeof(Token));
    source_insert_idx += overwrite_possible;

    if (overwrite_possible < overwrite_amount) {
        const size_t to_remove= overwrite_amount - overwrite_possible;
        memcpy(&source->data[source_insert_idx], &source->data[source_insert_idx + to_remove],
            (source->end_idx - to_remove - source_insert_idx + 1) * sizeof(Token));
        source->end_idx -= to_remove;
        source->current_idx -= to_remove;
    }

    const size_t source_tokens= source->end_idx - source->start_idx + 1;
    addition_tokens -= overwrite_possible;
    if (source_tokens + addition_tokens > source->max_tokens) {
        const size_t new_size= MAX(source_tokens + addition_tokens + 1, source_tokens << 1);
        resize_source(source, new_size);
    }

    if (addition_tokens == 0) return;

    assert(false && "Have not tested this line of combining");

    memmove(&source->data[source_insert_idx + addition_tokens], &source->data[source_insert_idx],
        (source->end_idx - source_insert_idx) * sizeof(Token)
    );
    memcpy(&source->data[source_insert_idx],
        &addition.data[addition.current_idx],
        addition_tokens * sizeof(Token)
    );

    source->end_idx += addition_tokens - 1;
}