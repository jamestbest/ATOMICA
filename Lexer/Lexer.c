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

#include "shared/Array.h"

static uint32_t current();
static uint32_t consume();
static uint32_t peek();
static uint32_t peer(size_t offset);

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

    Token_array_add(&tokens, (Token){.type= TT_EOTS, .buffer_index= cursor});

    print_all_tokens(&tokens);

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

uint8_t* get_token_end_ptr(const Token t) {
    return &buff.data[get_token_end(t.buffer_index)];
}

uint8_t* get_token_start_ptr(const Token t) {
    return &buff.data[t.buffer_index];
}

static Token create_simple_token(TokenType type) {
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

        case '\'': res= create_simple_token(TT_OPERATOR_CARROT); break;

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
            res= create_simple_token(TT_OPERATOR_BITWISE_AND);
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


