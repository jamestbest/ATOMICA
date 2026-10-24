//
// Created by jamescoward on 23/10/2026.
//

#include <stdbool.h>

#include "Parser.h"

#include "ParserInternal.h"
#include "Lexer/Token.h"
#include "Parserr/Parserr.h"

static size_t t_pos;
extern TokenArray tokens;

typedef struct {
    TokenArray tokens;
    NodeTypeArray types;
    NodeIndexArray lhs;
    NodeIndexArray rhs;
} NodeArray;

static NodeArray node_array;
static NodeIndexArray extra_array;

#define EXPECTED_NODE_COUNT 100

NodeRes parse() {
    t_pos= 0;

    setup_node_array();
    extra_array= NodeIndex_array_construct(EXPECTED_NODE_COUNT);

    const NodeIndex root= node_create(NT_ROOT, (Token){0});

    NodeIndexArray temp= NodeIndex_array_construct(10);
    while (t_pos < tokens.pos) {
        const NodeRes res= parse_top_level_statement();
        if (res.errcode != EC_SUCCESS) return NODE_FAIL(res.errcode);

        const NodeIndex ni= res.index;
        NodeIndex_array_add(&temp, ni);
    }

    NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(root);
}

NodeRes parse_variable_declaration() {
    // Buffer :: data {}
    // main :: (): !void
    // buff: Buffer'
    // cursor: n8

    const Token ident= consume();
    if (!EXPECT(TT_COLON)) {
        return unexpected(TT_COLON, current(),
            "Variable declaration requires colon after identifier");
    }

    NodeIndex type= NODE_INDEX_NONE;
    NodeIndex value= NODE_INDEX_NONE;
    if (EXPECT(TT_COLON)) {
        // this is implicit type
        // ::
    } else {
        const NodeRes type_res= parse_type();
        PROP_FAIL(type_res)

        type= type_res.index;

        if (!EXPECT(TT_COLON)) {
            goto parse_vd_no_value;
        }
    }

    const NodeRes value_res= parse_value();
    PROP_FAIL(value_res);

    value= value_res.index;

parse_vd_no_value:;
    const NodeIndex res= var_decl_node_create(ident, type, value);

    return NODE_SUCC(res);
}

#define TOKEN_AS_NODE_INDEX(token) (*(NodeIndex*)&token)

static NodeRes parse_type_errorable() {
    NodeIndex error_types= NODE_INDEX_NONE;

    const NodeIndex ni= node_create(NT_TYPE_ERRORABLE, consume());

    if (!EXPECT(TT_EXCLAMATION)) {
        Token identifier;
        if (!expect(TT_IDENTIFIER, &identifier)) {
            return unexpected(TT_IDENTIFIER, current(), "Currently error return types must be identifiers.\n");
        }

        error_types= TOKEN_AS_NODE_INDEX(identifier);

        if (!EXPECT(TT_EXCLAMATION)) {
            return unexpected(TT_EXCLAMATION, current(), "<<DEV>> How did we get here, we are parsing an errorable, but did not get an exclamation after identifier");
        }
    }

    const NodeRes return_type_res= parse_type();
    PROP_FAIL(return_type_res);

    // lhs is the error list, which is none in this case
    NodeIndex_array_set(&node_array.lhs, error_types, ni);
    NodeIndex_array_set(&node_array.rhs, return_type_res.index, ni);

    return NODE_SUCC(ni);
}

// current= >
static NodeRes parse_type_pointer() {
    Token ptr;
    if (!expect(TT_OPERATOR_MORE, &ptr)) return unexpected(TT_OPERATOR_MORE, current(), "<<DEVERROR>> tried to parse pointer type but did not get pointer at start.\n");

    NodeRes inner_type_res= parse_type();
    PROP_FAIL(inner_type_res)

    NodeIndex ni= node_create(NT_TYPE_POINTER, ptr);
    NodeIndex_array_set(&node_array.lhs, inner_type_res.index, ni);

    return NODE_SUCC(ni);
}

// todo: maybe could store the types in a row, with the type ending on a base type tag
static NodeRes parse_type() {
    // >        -X- pointer prefix
    // n8       -X- base type
    // Buffer   -X- identifier
    // '        -X- mark
    // ?        -X- nullable     -- postfix
    // !        -X- errorable    -- infix, error to the left, ret type to the right
    // []       -X- arrays
    // ()       -X- lambda header
    // data     - -                 todo
    // difference between :: and := todo
    // Errors!>i4

    NodeRes res;
    if (check(TT_OPERATOR_MORE)) {
        res= parse_type_pointer();
        goto parse_type_postfix;
    }

    if (peek().type == TT_EXCLAMATION || check(TT_EXCLAMATION)) {
        res= parse_type_errorable();
        goto parse_type_postfix;
    }

    Token tok;
    if (expect(TT_IDENTIFIER, &tok)) {
        const NodeIndex ni= node_create(NT_TYPE_LINK, tok);
        res= NODE_SUCC(ni);
        goto parse_type_postfix;
    }

    if (between(current().type, TT_INTERNAL_TYPE_START, TT_INTERNAL_TYPE_END)) {
        tok= consume();

        res= NODE_SUCC(node_create(NT_TYPE_BASE, tok));
        goto parse_type_postfix;
    }

    if (expect(TT_LBRACKET, &tok)) {
        NodeIndex array_node_index;

        switch (current().type) {
            case TT_DOT: {
                // [..]
                // lhs= inner type
                consume();

                if (!EXPECT(TT_DOT)) return unexpected(
                    TT_DOT, current(),
                    "Array syntax for dynamic arrays is [..], found only one dot.\n"
                );

                array_node_index= node_create(NT_TYPE_ARRAY_DYNAMIC, tok);

                break;
            }
            case TT_OPERATOR_MULT: {
                // [*]
                // lhs= inner type
                consume();

                array_node_index= node_create(NT_TYPE_ARRAY_LINK, tok);
                break;
            }
            case TT_TILDA: {
                // [~]
                // lhs= inner type
                consume();

                array_node_index= node_create(NT_TYPE_ARRAY_POOL, tok);
                break;
            }
            case TT_RBRACKET: {
                // [] slice
                // lhs= inner type

                array_node_index= node_create(NT_TYPE_ARRAY_SLICE, tok);
                break;
            }
            default: {
                // static array
                // lhs= inner type
                // rhs= size
                const NodeRes expr_res= parse_expr();
                PROP_FAIL(expr_res)

                array_node_index= node_create(NT_TYPE_ARRAY_STATIC, tok);
                NodeIndex_array_set(&node_array.rhs, expr_res.index, array_node_index);

                break;
            }
        }

        if (!EXPECT(TT_RBRACKET)) {
            return unexpected(TT_RBRACKET, current(), "Array type syntax must have a closing bracket, e.g. [..], did not find it.\n");
        }

        const NodeRes inner_type_res= parse_type();
        PROP_FAIL(inner_type_res);

        NodeIndex_array_set(&node_array.lhs, inner_type_res.index, array_node_index);

        res= NODE_SUCC(array_node_index);
        goto parse_type_postfix;
    }

    if (check(TT_LPAREN)) {
        res= parse_lambda_header();
        goto parse_type_postfix;
    }

    parse_error("Unable to parse type. Starter token is not valid\n");
    print_token(current(), -1, -1);
    return NODE_FAIL(EC_FAIL);

parse_type_postfix:
    PROP_FAIL(res)

    // check for marks and nullables
    while (true) {
        if (expect(TT_QUESTION, &tok)) {
            NodeIndex node= node_create(NT_TYPE_NULLABLE, tok);
            NodeIndex_array_set(&node_array.lhs, res.index, node);
            res.index= node;
        } else if (expect(TT_CARROT, &tok)) {
            NodeIndex node= node_create(NT_TYPE_MARKED, tok);
            NodeIndex_array_set(&node_array.lhs, res.index, node);
            res.index= node;
        } else break;
    }

    return res;
}

// starts with current= (
// todo: default values
// todo: generics
NodeRes parse_lambda_header() {
    // token: paren
    // lhs: extra start
    // rhs: extra end
    //
    // extra contains
    // Param 0
    // ..
    // Param n-1
    // return type

    // (<ident>: <type>, <ident>: <type>): <type>
    Token paren_token;
    if (!expect(TT_LPAREN, &paren_token)) return unexpected(TT_LPAREN, current(), "<<DEVERR>> tried parsing lambda header without lparen at start.\n");

    NodeIndexArray params= NodeIndex_array_construct(4);

    if (EXPECT(TT_RPAREN)) goto parse_lambda_header_no_params;

    do {
        // token: ident
        // lhs: type
        // rhs: default value | NONE

        Token ident;
        if (!expect(TT_IDENTIFIER, &ident)) {
            return unexpected(TT_IDENTIFIER, current(), "Lambda header parameters must start with an identifier. E.g. x: i4\n");
        }

        if (!EXPECT(TT_COLON)) return unexpected(TT_COLON, current(), "Lambda header parameters must have a colon after the name.\n");

        // todo: support := default_value
        NodeRes type_res= parse_type();
        PROP_FAIL(type_res);

        NodeIndex param_index= node_create(NT_PARAM, ident);
        NodeIndex_array_set(&node_array.lhs, type_res.index, param_index);
        NodeIndex_array_set(&node_array.rhs, NODE_INDEX_NONE, param_index);

        NodeIndex_array_add(&params, param_index);
    } while (EXPECT(TT_COMMA));

parse_lambda_header_no_params:;
    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(), "Lambda param list must be ended with a right parenthesis i.e. ')'\n");
    }

    const NodeRes return_type_res= parse_type();
    PROP_FAIL(return_type_res);

    const NodeIndex header_index= node_create(NT_LAMBDA_HEADER, paren_token);
    const NodeIndex extra_start= NodeIndex_array_add_bulk(&extra_array, params.data, params.pos);
    const NodeIndex extra_end= extra_start + params.pos;

    NodeIndex_array_set(&node_array.lhs, extra_start, header_index);
    NodeIndex_array_set(&node_array.rhs, extra_end, header_index);

    return NODE_SUCC(header_index);
}

NodeRes parse_import_statement() {
    assert(false);
}

static NodeRes parse_top_level_statement() {
    Token t= current();

    switch (t.type) {
        case TT_IDENTIFIER: return parse_variable_declaration();
        case TT_KEYWORD_IMPORT: return parse_import_statement();
        default: {
            parse_error("Expected variable declaration or import statement at top level of program. Found: ");
            print_token(t, -1, -1);
            return NODE_FAIL(EC_FAIL);
        }
    }
}

static void setup_node_array() {
    node_array.tokens= Token_array_construct(EXPECTED_NODE_COUNT);
    node_array.types= NodeType_array_construct(EXPECTED_NODE_COUNT);
    node_array.lhs= NodeIndex_array_construct(EXPECTED_NODE_COUNT);
    node_array.rhs= NodeIndex_array_construct(EXPECTED_NODE_COUNT);

    node_array.tokens.pos= 1;
    node_array.types.pos= 1;
    node_array.lhs.pos= 1;
    node_array.rhs.pos= 1;
}

static void destroy_node_array() {
    Token_array_destroy(&node_array.tokens);
    NodeType_array_destroy(&node_array.types);
    NodeIndex_array_destroy(&node_array.lhs);
    NodeIndex_array_destroy(&node_array.rhs);
}

static NodeIndex node_create(const NodeType type, const Token token) {
    const size_t old_max= node_array.types.max_elements;
    NodeType_array_add(&node_array.types, type);

    if (node_array.types.max_elements > old_max) {
        array_resize((Array*)&node_array.tokens, node_array.types.max_elements);
        array_resize((Array*)&node_array.lhs, node_array.types.max_elements);
        array_resize((Array*)&node_array.rhs, node_array.types.max_elements);
    }

    Token_array_add(&node_array.tokens, token);

    node_array.lhs.pos++;
    node_array.rhs.pos++;

    return node_array.types.pos - 1;
}

static NodeIndex var_decl_node_create(Token ident, NodeIndex type, NodeIndex value) {
    NodeType_array_add(&node_array.types, NT_VAR_DECL);
    Token_array_add(&node_array.tokens, ident);
    NodeIndex_array_add(&node_array.lhs, type);
    NodeIndex_array_add(&node_array.rhs, value);

    return node_array.types.pos - 1;
}

static VarDeclNode var_decl_node_get(NodeIndex index) {
    if (index >= node_array.types.pos) assert(false);

    return (VarDeclNode) {
        .token= Token_array_get(&node_array.tokens, index),
        .type= NodeIndex_array_get(&node_array.lhs, index),
        .value= NodeIndex_array_get(&node_array.rhs, index)
    };
}

static Token current() {
    if (t_pos >= tokens.pos) return (Token) {
        .type= TT_EOTS,
        .index= -1
    };

    return Token_array_get(&tokens, t_pos);
}

static Token consume() {
    const Token res= current();
    t_pos++;
    return res;
}

static Token peek() {
    return peer(1);
}

static Token peer(size_t offset) {
    if (t_pos + offset >= tokens.pos) return (Token) {
        .type= TT_EOTS,
        .index= -1
    };

    return Token_array_get(&tokens, t_pos + offset);
}

static bool check(const TokenType type) {
    return (current().type == type);
}

static bool expect(const TokenType type, Token* out) {
    if (current().type == type) {
        if (out) {
            *out= consume();
            return true;
        }

        consume();
        return true;
    }

    return false;
}

static inline bool between(TokenType type, TokenType start_exclusive, TokenType end_exclusive) {
    return (start_exclusive < type) && (type < end_exclusive);
}

#undef EXPECT
#undef EXPECTED_NODE_COUNT
