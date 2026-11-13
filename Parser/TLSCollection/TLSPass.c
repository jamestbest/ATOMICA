//
// Created by jamescoward on 10/11/2026.
//

#include "TLSPass.h"

#include <stdio.h>

#include "Gluon/Gluon.h"
#include "Lexer/Token.h"

static bool is_valid_tls_start(const MyTokenType type) {
    switch (type) {
        case TT_IDENTIFIER:
        case TT_KEYWORD_ENTRY:
        case TT_KEYWORD_IMPORT:
            return true;

        case TT_HASH:
            return true;

        case TT_NEWLINE:
        case TT_DELIMITER:
        case TT_EOTS: return false;
        default:
            return false;
    }
}

__thread size_t line= 0;
__thread size_t idx= 0;
__thread TokenArray tokens;

static Token current() {
    if (idx >= tokens.pos) return (Token) {
        .type= TT_EOTS
    };

    return Token_array_get(&tokens, idx);
}

static Token consume() {
    const Token t= current();
    if (t.type == TT_NEWLINE) line++;
    idx++;
    return t;
}

static bool is_delimiter(const MyTokenType type) {
    return type == TT_NEWLINE || type == TT_DELIMITER;
}

static void consume_tls_tokens() {
    Token t;
    while (t= current(), !is_delimiter(t.type) && t.type != TT_LBRACE) {
        if (t.type == TT_EOTS) break;
        consume();
    }

    size_t lbrace_count= 0;
    if (t.type == TT_LBRACE) {
        do {
            t= Token_array_get(&tokens, idx);
            if (t.type == TT_EOTS) break;
            if (t.type == TT_LBRACE) lbrace_count++;
            else if (t.type == TT_RBRACE) lbrace_count--;

            consume();
        } while ((lbrace_count != 0));
    }
}

static void eat_all_delimiters() {
    Token c;
    while (c= current(), is_delimiter(c.type)) consume();
}

// split the token array into TokenSources for each tls
//  and then pass these to the orchestrator for parsing
int collect_tls(const TokenArray tokens_) {
    tokens= tokens_;
    idx= 0;
    line= 0;

    while (idx < tokens.pos) {
        eat_all_delimiters();

        const Token t= current();

        if (is_valid_tls_start(t.type)) {
            const size_t start= idx;
            const size_t start_line= line;

            consume_tls_tokens();
            const size_t end= idx;

            const MyTokenSource source= (MyTokenSource) {
                .data= tokens.data,
                .current_idx= start,
                .start_idx= start,
                .end_idx= end,
                .start_line= start_line
            };

            printf("Creating parse job for tls with %zu tokens starting line %zu.\n", end - start + 1, start_line);
            add_job(create_parse_job(source, NO_DEPENDENCY));
        }
    }

    return EC_SUCCESS;
}