//
// Created by jamescoward on 14/10/2026.
//

#include "Lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Token.h"

#include "shared/Array.h"

static uint32_t current();
static uint32_t consume();
static uint32_t peek();

static bool is_alpha(uint32_t character);
static bool is_numeric(uint32_t character);
static bool is_numeric_base(uint32_t character, uint8_t base);
static bool is_alpha_numeric(uint32_t character);
static Token lex_string();

static int read_file_into_buffer(const char* file_path, Buffer* out) {
    FILE* file= fopen(file_path, "r");
    if (!file) return EXIT_FAILURE;

    assert(fseek(file, 0, SEEK_END) == 0);
    const long file_size= ftell(file);
    assert(fseek(file, 0, SEEK_SET) == 0);

    out->data= malloc(file_size + 1);
    if (!out->data) assert(false);

    const size_t bytes_read= fread(out->data, 1, file_size, file);
    out->data[bytes_read]= '\0';

    out->size= bytes_read;
    out->pos= bytes_read;

    return EXIT_SUCCESS;
}

static Buffer buff;
static size_t cursor;

ARRAY_ADD(Token, Token)
extern TokenArray tokens;

static Token lex_token();
static Token lex_comment();
static Token lex_identifier();
static Token lex_numeric_literal();
static void print_all_tokens();

int lex(const char* file_path) {
    tokens= Token_array_construct(5);

    const int res= read_file_into_buffer(file_path, &buff);
    if (res != EXIT_SUCCESS) return res;

    cursor= 0;

    while (cursor < buff.pos) {
        const Token t= lex_token();
        if (t.type == TT_IGNORE) continue;
        if (t.type == TT_ERROR) {
            perror("lexer found error.\n");
            return EXIT_FAILURE;
        }

        Token_array_add(&tokens, t);
    }

    Token_array_add(&tokens, (Token){.type= TT_EOTS, .index= cursor});

    print_all_tokens();

    return EXIT_SUCCESS;
}

static size_t get_token_end(size_t start) {
    const size_t cursor_save= cursor;

    cursor= start;
    lex_token();
    const size_t res= cursor;
    cursor= cursor_save;

    return res;
}

inline uint8_t* get_token_end_ptr(const Token t) {
    return &buff.data[get_token_end(t.index)];
}

inline uint8_t* get_token_start_ptr(const Token t) {
    return &buff.data[t.index];
}

static void print_all_tokens() {
    size_t line= 1, col= 1;
    size_t line_index= 0;

    size_t t_pos= 0;
    while (t_pos < tokens.pos) {
        const Token t= Token_array_get(&tokens, t_pos);

        bool is_nl= buff.data[t.index] == '\n';
        if (is_nl) {
            line++;
            col= 1;
            line_index= t.index;
        }
        col= t.index - line_index;

        print_token(t, line, col);
        printf("\n");

        t_pos++;
    }
}

static Token create_simple_token(TokenType type) {
    return (Token) {
        .type= type,
        .index= cursor
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

        case '\'': res= create_simple_token(TT_CARROT); break;

        case ':': res= create_simple_token(TT_COLON); break;
        case ',': res= create_simple_token(TT_COMMA); break;
        case '~': res= create_simple_token(TT_TILDA); break;
        case '.': res= create_simple_token(TT_DOT); break;

        case '!': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_NEQ);
                consume();
                break;
            }
            res= create_simple_token(TT_EXCLAMATION);
            break;
        }
        case '?': res= create_simple_token(TT_QUESTION); break;
        case '#': res= create_simple_token(TT_HASH); break;
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

        case '+': {
            if (peek() == '+') {
                res= create_simple_token(TT_OPERATOR_PLUS_PLUS);
                consume();
            } else if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_PLUS);
                consume();
            } else {
                res= create_simple_token(TT_OPERATOR_PLUS);
            }

            break;
        }

        case '-': {
            if (peek() == '-') {
                res= create_simple_token(TT_OPERATOR_MINUS_MINUS);
                consume();
            } else if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_ASSIGN_SUBTRACT);
                consume();
            } else {
                res= create_simple_token(TT_OPERATOR_SUBTRACT);
            }

            break;
        }

        case '>': {
            if (peek() == '=') {
                res= create_simple_token(TT_OPERATOR_MORE_EQ);
                consume();
                break;
            }

            res= create_simple_token(TT_OPERATOR_MORE);
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

        case '\n': res= create_simple_token(TT_DELIMITER); break;
        case '\t':
        case ' ': res= create_simple_token(TT_IGNORE);
            break;

        default:
            goto lex_cont;
    }

    consume();
    return res;

lex_cont:
    if (c == '/' && peek() == '/') {
        return lex_comment();
    }

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

    printf("Did not match on character '%c' v: %d.\n", current(), current());

    return (Token){.type= TT_IGNORE};
}

static Token lex_comment() {
    consume();
    consume();

    const Token res= create_simple_token(TT_COMMENT);

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
    const size_t end= cursor;
    const size_t ident_bytes= end - start;

    const char* start_ptr= (const char*)&buff.data[start];

#define MATCH_RES(en) { res.type= en; return res; }
#define MATCH_CASE(kw, en) if(strncmp(start_ptr, kw, ident_bytes) == 0) MATCH_RES(en)
    switch (start_char) {
        case 'b': {
            MATCH_CASE("break", TT_KEYWORD_BREAK)
            MATCH_CASE("bool", TT_TYPE_BOOL)
            break;
        }
        case 'c': {
            MATCH_CASE("continue", TT_KEYWORD_CONTINUE)
            break;
        }
        case 'd': {
            MATCH_CASE("defer", TT_KEYWORD_DEFER)
            MATCH_CASE("data", TT_KEYWORD_DATA)
            break;
        }
        case 'e': {
            MATCH_CASE("else", TT_KEYWORD_ELSE)
            MATCH_CASE("entry", TT_KEYWORD_ENTRY)
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
            MATCH_CASE("mark", TT_KEYWORD_MARK)
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
            break;
        }
        case 'u': {
            MATCH_CASE("unmark", TT_KEYWORD_UNMARK)
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

#undef IS_KW
#undef MATCH_CASE
}

static Token lex_string() {
    const Token res= create_simple_token(TT_LIT_STRING);

    consume(); // eat first "

    while (current() != '"') {
        if (current() == '\n') {
            perror("Strings must be on a single line. Thanks\n");
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


