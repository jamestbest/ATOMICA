//
// Created by jamescoward on 14/10/2026.
//

#include "Lexer.h"

#include "Error.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Token.h"
#include "Gluon/Gluon.h"
#include "Gluon/File/File.h"

#include "shared/Array.h"
#include "Logger/Logger.h"

#define LEX_SUCC(x) (LexRes){.tokens= x, .errcode= EC_SUCCESS}
#define LEX_FAIL (LexRes){.errcode= EC_FAIL}

static uint32_t current();
static uint32_t consume();
static uint32_t peek();
static uint32_t peer(size_t offset);

static bool is_alpha(uint32_t character);
static bool is_numeric(uint32_t character);
static bool is_numeric_base(uint32_t character, uint8_t base);
static bool is_alpha_numeric(uint32_t character);
static Token lex_string();

ARRAY_ADD(Token, Token)

static __thread size_t cursor;
static __thread Buffer buff;

static Token lex_token();
static Token lex_comment();
static Token lex_identifier();
static Token lex_numeric_literal();
static Token lex_directives();

LexRes lex(const char* file_path) {
    TokenArray tokens= Token_array_construct(5);

    const int ec= read_file_into_buffer(file_path, &buff);
    if (ec != EXIT_SUCCESS) return LEX_FAIL;

    cursor= 0;

    while (cursor < buff.pos) {
        const Token t= lex_token();
        if (t.type == TT_IGNORE) continue;
        if (t.type == TT_ERROR) {
            logger_error("lexer found error.\n");
            return LEX_FAIL;
        }

        Token_array_add(&tokens, t);
    }

    Token_array_add(&tokens, (Token){.type= TT_EOTS, .buffer_index= cursor});

    const MyTokenSource source= (MyTokenSource) {
        .data= tokens.data,
        .current_idx= 0,
        .start_idx= 0,
        .end_idx= tokens.pos - 1,
        .start_line= 0
    };
    print_all_tokens(source);

    // todo: add guard to this
    FileInfo_arrayg_ptr(&file_infos, current_job.file_id)->buffer= buff;

    return (LexRes){.tokens= tokens, .errcode= EC_SUCCESS};
}

void set_lex_buffer(Buffer buff_) {
    buff= buff_;
}

static size_t get_token_end(size_t start) {
    const size_t cursor_save= cursor;

    cursor= start;
    lex_token();
    const size_t res= cursor;
    cursor= cursor_save;

    return res;
}

uint8_t* get_token_end_ptr(const Token t) {
    return &buff.data[get_token_end(t.buffer_index)];
}

uint8_t* get_token_start_ptr(const Token t) {
    return &buff.data[t.buffer_index];
}

static Token create_simple_token(MyTokenType type) {
    return (Token) {
        .type= type,
        .buffer_index= cursor
    };
}

Token lex_token() {
    const uint32_t c= current();

    Token res;
    switch (c) {
        case '\0': res= create_simple_token(TT_EOTS); break;
        case '[': res= create_simple_token(TT_LBRACKET); break;
        case ']': res= create_simple_token(TT_RBRACKET); break;
        case '(': res= create_simple_token(TT_LPAREN); break;
        case ')': res= create_simple_token(TT_RPAREN); break;
        case '{': res= create_simple_token(TT_LBRACE); break;
        case '}': res= create_simple_token(TT_RBRACE); break;

        case '$': res= create_simple_token(TT_DOLLAR); break;

        case '\'': res= create_simple_token(TT_OPERATOR_POSTFIX_CARROT); break;

        case ':': res= create_simple_token(TT_COLON); break;
        case ',': res= create_simple_token(TT_COMMA); break;
        case '~': res= create_simple_token(TT_OPERATOR_CAST); break;
        case '.': res= create_simple_token(TT_OPERATOR_AMBIGUOUS_DOT); break;

        case '!': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_NEQ);
                consume();
                break;
            }
            res= create_simple_token(TT_OPERATOR_AMBIGUOUS_EXCLAMATION);
            break;
        }
        case '?': res= create_simple_token(TT_OPERATOR_AMBIGUOUS_QUESTION); break;
        case '#': return lex_directives();
        case ';': res= create_simple_token(TT_DELIMITER); break;

        case '=': {
            if (peek() == '=') {
                consume();
                res= create_simple_token(TT_OPERATOR_EQ);
                break;
            }
            res= create_simple_token(TT_OPERATOR_ASSIGN);
            break;
        }

        case '*': {
            // *
            // *=
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_MULT);
                consume();
                break;
            }
            res= create_simple_token(TT_OPERATOR_MULT);
            break;
        }

        case '/': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_DIV);
                consume();
                break;
            }
            if (peek() == '/') {
                return lex_comment();
            }
            res= create_simple_token(TT_OPERATOR_DIV);
            break;
        }

        case '%': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_MOD);
                consume();
                break;
            }
            res= create_simple_token(TT_OPERATOR_MOD);
            break;
        }

        case '&': {
            if (peek() == '&') {
                res= create_simple_token(TT_OPERATOR_LOGICAL_AND);
                consume();
                break;
            }
            res= create_simple_token(TT_OPERATOR_BITWISE_AND);
            break;
        }

        case '|': {
            if (peek() == '|') {
                res= create_simple_token(TT_OPERATOR_LOGICAL_OR);
                consume();
                break;
            }
            if (peek() == ':') {
                res= create_simple_token(TT_OPERATOR_TERNARY_CAPTURE);
                consume();
                break;
            }
            if (peek() == '-') {
                if (peer(2) == '|') {
                    res= create_simple_token(TT_OPERATOR_LOGICAL_XOR);
                    consume();
                    consume();
                    break;
                }
                res= create_simple_token(TT_OPERATOR_BITWISE_XOR);
                consume();
                break;
            }
            res= create_simple_token(TT_OPERATOR_BINARY_BITWISE_OR);
            break;
        }

        case '+': {
            if (peek() == '+') {
                res= create_simple_token(TT_OPERATOR_AMBIGUOUS_PLUS_PLUS);
                consume();
            } else if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_PLUS);
                consume();
            } else {
                res= create_simple_token(TT_OPERATOR_AMBIGUOUS_PLUS);
            }

            break;
        }

        case '-': {
            if (peek() == '-') {
                res= create_simple_token(TT_OPERATOR_AMBIGUOUS_MINUS_MINUS);
                consume();
            } else if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_SUBTRACT);
                consume();
            } else {
                res= create_simple_token(TT_OPERATOR_AMBIGUOUS_SUBTRACT);
            }

            break;
        }

        case '>': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_MORE_EQ);
                consume();
                break;
            }

            res= create_simple_token(TT_OPERATOR_AMBIGUOUS_MORE);
            break;
        }

        case '<': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_LESS_EQ);
                consume();
                break;
            }

            res= create_simple_token(TT_OPERATOR_LESS);
            break;
        }

        case '\r': {
            if (peek() == '\n') {
                consume();
                res= create_simple_token(TT_NEWLINE);
                break;
            }
            assert(false);
        }

        case '\n': res= create_simple_token(TT_NEWLINE); break;
        case '\t':
        case ' ': res= create_simple_token(TT_IGNORE);
            break;

        default:
            goto lex_cont;
    }

    consume();
    return res;

lex_cont:
    if (is_alpha(c)) {
        // identifier
        // some check for keywords
        return lex_identifier();
    }

    if (is_numeric(c)) {
        return lex_numeric_literal();
        // literal numbers
    }

    if (c == '\"') {
        return lex_string();
    }

    logger_fadd("Did not match on character '%c' v: %d.\n", current(), current());

    return (Token){.type= TT_ERROR};
}

static Token lex_directives() {
    Token res= create_simple_token(TT_INTERNAL_DIRECTIVE_START);

    consume(); // eat the hash

    const size_t start= cursor;
    const uint32_t start_char= current();

    while (is_alpha(current())) consume();
    const uint32_t end= cursor;

#define MATCH_RES(en) { res.type= en; return res; }
#define MATCH_CASE(kw, en) if(strncmp(start_ptr, kw, (sizeof(kw) - 1)) == 0) MATCH_RES(en)

    const char* start_ptr= (const char*)&buff.data[start];
    switch (start_char) {
        case 'i': {
            MATCH_CASE("insert", TT_DIRECTIVE_INSERT)
            break;
        }
        case 'r': {
            MATCH_CASE("run", TT_DIRECTIVE_RUN)
            break;
        }
        default: break;
    }

    logger_errorf("Unknown directive found %.*s", end - start + 1, start_ptr);
    return create_simple_token(TT_ERROR);
#undef MATCH_RES
#undef MATCH_CASE
}

static Token lex_comment() {
    consume();
    consume();

    const Token res= create_simple_token(TT_IGNORE);

    while (current() != '\n') {
        consume();
    }

    return res;
}

static Token lex_identifier() {
    const size_t start= cursor;
    const uint32_t start_char= current();

    Token res= create_simple_token(TT_IDENTIFIER);

    uint32_t c;
    while (c= current(), is_alpha_numeric(c) || c == '_') consume();

    const char* start_ptr= (const char*)&buff.data[start];

#define MATCH_RES(en) { res.type= en; return res; }
#define MATCH_CASE(kw, en) if(strncmp(start_ptr, kw, (sizeof(kw) - 1)) == 0) MATCH_RES(en)
    switch (start_char) {
        case 'a': {
            MATCH_CASE("as", TT_OPERATOR_CAST)
            break;
        }
        case 'b': {
            MATCH_CASE("break", TT_KEYWORD_BREAK)
            MATCH_CASE("bool", TT_TYPE_BOOL)
            break;
        }
        case 'c': {
            MATCH_CASE("continue", TT_KEYWORD_CONTINUE)
            MATCH_CASE("const", TT_KEYWORD_CONST)
            MATCH_CASE("code", TT_TYPE_CODE)
            break;
        }
        case 'd': {
            MATCH_CASE("defer", TT_KEYWORD_DEFER)
            MATCH_CASE("data", TT_KEYWORD_DATA)
            MATCH_CASE("do", TT_KEYWORD_DO)
            break;
        }
        case 'e': {
            MATCH_CASE("else", TT_KEYWORD_ELSE)
            MATCH_CASE("entry", TT_KEYWORD_ENTRY)
            MATCH_CASE("export", TT_KEYWORD_EXPORT)
            break;
        }
        case 'f': {
            MATCH_CASE("for", TT_KEYWORD_FOR)
            MATCH_CASE("false", TT_BOOLEAN_FALSE)
            break;
        }
        case 'i': {
            MATCH_CASE("if", TT_KEYWORD_IF)
            MATCH_CASE("import", TT_KEYWORD_IMPORT)
            MATCH_CASE("id", TT_TYPE_ID)
            switch (*(start_ptr + 1)) {
                case '1': MATCH_RES(TT_TYPE_I1)
                case '2': MATCH_RES(TT_TYPE_I2)
                case '4': MATCH_RES(TT_TYPE_I4)
                case '8': MATCH_RES(TT_TYPE_I8)
                default: break;
            }
            break;
        }
        case 'n': {
            MATCH_CASE("null", TT_NULL)
            switch (*(start_ptr + 1)) {
                case '1': MATCH_RES(TT_TYPE_N1)
                case '2': MATCH_RES(TT_TYPE_N2)
                case '4': MATCH_RES(TT_TYPE_N4)
                case '8': MATCH_RES(TT_TYPE_N8)
                default: break;
            }
            break;
        }
        case 'm': {
            MATCH_CASE("mark", TT_OPERATOR_PREFIX_MARK)
            break;
        }
        case 'q': {
            switch (*(start_ptr + 1)) {
                case '2': MATCH_RES(TT_TYPE_I2)
                case '4': MATCH_RES(TT_TYPE_I4)
                case '8': MATCH_RES(TT_TYPE_I8)
                default: break;
            }
            MATCH_CASE("q16", TT_TYPE_Q16)
            break;
        }
        case 'r': {
            MATCH_CASE("ret", TT_KEYWORD_RET)
            break;
        }
        case 's': {
            MATCH_CASE("string", TT_TYPE_STRING);
            break;
        }
        case 't': {
            MATCH_CASE("true", TT_BOOLEAN_TRUE);
            MATCH_CASE("to", TT_KEYWORD_TO);
            break;
        }
        case 'u': {
            MATCH_CASE("unmark", TT_OPERATOR_PREFIX_UNMARK)
            break;
        }
        case 'v': {
            MATCH_CASE("void", TT_TYPE_VOID)
            break;
        }
        case 'w': {
            MATCH_CASE("while", TT_KEYWORD_WHILE)
            MATCH_CASE("with", TT_KEYWORD_WITH)
            break;
        }
        case 'y': {
            MATCH_CASE("yield", TT_KEYWORD_YIELD)
            break;
        }
        default: break;
    }

    return res;

#undef MATCH_RES
#undef MATCH_CASE
}

static Token lex_string() {
    const Token res= create_simple_token(TT_LIT_STRING);

    consume(); // eat first "

    while (current() != '"') {
        if (current() == '\n') {
            logger_error("Strings must be on a single line. Thanks\n");
            return create_simple_token(TT_ERROR);
        }
        consume();
    }

    consume(); // eat the last "

    return res;
}

static Token lex_numeric_literal() {
    // int literals
    // float literals
    char* int_end= NULL;
    char* float_end= NULL;
    const char* start= (char*)&buff.data[cursor];

    long long int_res= strtoll(start , &int_end, 0);
    long double float_res= strtold(start, &float_end);

    // todo: error check

    Token res;
    if (float_end > int_end) {
        res= create_simple_token(TT_LIT_FLOAT);
        cursor += (float_end - start);
    } else {
        res= create_simple_token(TT_LIT_INT);
        cursor += (int_end - start);
    }

    return res;
}

static bool is_alpha(const uint32_t character) {
    return ((character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z')
    );
}

static bool is_numeric(const uint32_t character) {
    return (character >= '0' && character <= '9');
}

static bool is_numeric_base(const uint32_t character, const uint8_t base) {
    assert(base <= 16);
    assert(base >= 2);

    if (base <= 10) {
        return (character - '0') < base;
    }

    return is_numeric(character) || (
        (character - 'a') < (base - 10)
    ) || (
        (character - 'A') < (base - 10)
    );
}

static bool is_alpha_numeric(const uint32_t character) {
    return is_alpha(character) || is_numeric(character);
}

typedef struct {
    uint32_t character;
    uint32_t size;
} UTF8Res;

static UTF8Res read_utf8_character(const size_t start) {
    const uint8_t first= buff.data[start];

    if ((first & 0x80) == 0) {
        // one byte only
        return (UTF8Res) {
            .size= 1,
            .character= first
        };
    }

    const uint8_t second= buff.data[start + 1];
    if ((first & 0xE0) == 0xC0) {
        return (UTF8Res) {
            .size= 2,
            .character= (
                ((first & 0x1f) << 6) |
                (second & 0x3f)
            )
        };
    }

    const uint8_t third= buff.data[start + 2];
    if ((first & 0xF0) == 0xE0) {
        return (UTF8Res) {
            .size= 3,
            .character= (
                ((first & 0x0f) << 12) |
                ((second & 0x3f) << 6) |
                ((third & 0x3f))
            )
        };
    }

    const uint8_t forth= buff.data[start + 3];
    if ((first & 0xF8) == 0xF0) {
        return (UTF8Res) {
            .size= 4,
            .character= (
                ((first & 0x07) << 18) |
                ((second & 0x3f) << 12) |
                ((third & 0x3f) << 6) |
                ((forth & 0x3f))
            )
        };
    }

    return (UTF8Res) {
        .character= buff.data[start],
        .size= 1
    };
}

uint32_t current() {
    return read_utf8_character(cursor).character;
}

uint32_t consume() {
    const UTF8Res res= read_utf8_character(cursor);
    cursor += res.size;

    return res.character;
}

uint32_t peek() {
    const UTF8Res res_0= read_utf8_character(cursor);
    const UTF8Res res_1= read_utf8_character(cursor + res_0.size);

    return res_1.character;
}

uint32_t peer(size_t offset) {
    assert(offset > 0);

    size_t c_cursor= cursor;
    UTF8Res res;
    while (offset > 0) {
        res= read_utf8_character(c_cursor);
        offset--;
        c_cursor+= res.size;
    }

    return res.character;
}


const char* lex_get_string(const size_t buffer_index_start) {
    const FileInfo info= FileInfo_arrayg_get(&file_infos, current_job.file_id);
    buff= info.buffer;
    const size_t end= get_token_end(buffer_index_start);
    const size_t size= end - buffer_index_start - 2;

    uint8_t* res= malloc(size + 1);
    memcpy(res, &info.buffer.data[buffer_index_start] + 1, size);
    res[size]= '\0';

    return (char*)res;
}


