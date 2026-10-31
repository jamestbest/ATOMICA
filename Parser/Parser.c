//
// Created by jamescoward on 23/10/2026.
//

#include <stdbool.h>

#include "Parser.h"

#include "ParserInternal.h"
#include "Parser/SharedParsing.h"
#include "Lexer/Token.h"
#include "Parserr/Parserr.h"
#include "shared/String.h"
#include "Shunt/ShuntingYard.h"

// todo: _add functions return u64, but we store u32s
// todo: zero init node_array or write INDEX_NONE (don't do that)
// todo: eat delimiters in braces

static size_t t_pos;

NodeArray node_array;
// if lhs or rhs are NODE_INDEX_NONE then there is no data in the extra_array
NodeIndexArray extra_array;

#define EXPECTED_NODE_COUNT 100
// _Static_assert(EXPECTED_NODE_COUNT > 1, "");

NodeRes parse() {
    t_pos= 0;

    setup_node_array();
    extra_array= NodeIndex_array_construct(EXPECTED_NODE_COUNT);
    extra_array.pos++;

    const NodeIndex root= node_create(NT_ROOT, (Token){0});

    NodeIndexArray temp= NodeIndex_array_construct(10);
    while (t_pos < tokens.pos) {
        const NodeRes res= parse_top_level_statement();
        if (res.errcode != EC_SUCCESS) return NODE_FAIL(res.errcode);
        print_node(res.index);

        const NodeIndex ni= res.index;
        NodeIndex_array_add(&temp, ni);
    }

    NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(root);
}

static NodeRes parse_variable_declaration() {
    // Buffer :: data {}
    // main :: (): !void
    // buff: Buffer'
    // cursor: n8
    // could be const or var; :: :=
    // todo support : const=
    //     - const
    //     - '      (maybe)
    //     - ?      (probably not)

    const Token ident= consume();
    if (!EXPECT(TT_COLON)) {
        return unexpected(TT_COLON, current(),
            "Variable declaration requires colon after identifier");
    }

    NodeIndex type= NODE_INDEX_NONE;
    NodeIndex value= NODE_INDEX_NONE;
    NodeType node_type= NT_VAR_DECL;
    if (EXPECT(TT_COLON)) {
        // this is implicit type
        // ::
        // and const
        node_type= NT_CONST_DECL;
    } else if (EXPECT(TT_OPERATOR_ASSIGN)) {
        // this is implicit type
        // and variable
    } else {
        if (check(TT_KEYWORD_CONST) && (
            peek().type == TT_OPERATOR_ASSIGN ||
            peek().type == TT_COLON
        )) {
            type= node_create(NT_TYPE_CONST, consume());
        } else {
            const NodeRes type_res= parse_type();
            PROP_FAIL(type_res)

            type= type_res.index;
        }

        bool is_const_decl;
        if (is_const_decl= EXPECT(TT_COLON), !is_const_decl && !EXPECT(TT_OPERATOR_ASSIGN)) {
            goto parse_vd_no_value;
        }

        node_type= is_const_decl? NT_CONST_DECL: NT_VAR_DECL;
    }

    const NodeRes value_res= parse_expr(false);
    PROP_FAIL(value_res);

    value= value_res.index;

parse_vd_no_value:;
    const NodeIndex ni= node_create(node_type, ident);
    SET_LHS(type, ni);
    SET_RHS(value, ni);

    return NODE_SUCC(ni);
}

#define TOKEN_AS_NODE_INDEX(token) (*(NodeIndex*)&token)

static NodeRes parse_type_errorable() {
    NodeIndex error_types= NODE_INDEX_NONE;

    const NodeIndex ni= node_create(NT_TYPE_ERRORABLE, consume());

    if (!EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
        Token identifier;
        if (!expect(TT_IDENTIFIER, &identifier)) {
            return unexpected(TT_IDENTIFIER, current(), "Currently error return types must be identifiers.\n");
        }

        error_types= TOKEN_AS_NODE_INDEX(identifier);

        if (!EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
            return unexpected(TT_OPERATOR_AMBIGUOUS_EXCLAMATION, current(), "<<DEV>> How did we get here, we are parsing an errorable, but did not get an exclamation after identifier");
        }
    }

    const NodeRes return_type_res= parse_type();
    PROP_FAIL(return_type_res);

    // lhs is the error list, which is none in this case
    SET_LHS(error_types, ni);
    SET_RHS(return_type_res.index, ni);

    return NODE_SUCC(ni);
}

// current= >
static NodeRes parse_type_pointer() {
    Token ptr;
    if (!expect(TT_OPERATOR_AMBIGUOUS_MORE, &ptr)) return unexpected(TT_OPERATOR_AMBIGUOUS_MORE, current(), "<<DEVERROR>> tried to parse pointer type but did not get pointer at start.\n");

    NodeRes inner_type_res= parse_type();
    PROP_FAIL(inner_type_res)

    NodeIndex ni= node_create(NT_TYPE_POINTER, ptr);
    SET_LHS(inner_type_res.index, ni);

    return NODE_SUCC(ni);
}

// <ident>: <type>
// NT_TYPED_IDENTIFIER
static NodeRes parse_typed_identifier() {
    Token ident;
    if (!expect(TT_IDENTIFIER, &ident)) {
        return unexpected(TT_IDENTIFIER, current(), "Data field must start with an identifier, followed by a type after a colon, e.g. x: i4.\n");
    }

    if (!EXPECT(TT_COLON)) {
        return unexpected(TT_COLON, current(), "Data field must start with an identifier, followed by a type after a colon, e.g. x: i4.\n");
    }

    const NodeRes type_res= parse_type();
    PROP_FAIL(type_res)

    const NodeIndex ni= node_create(NT_TYPED_IDENTIFIER, ident);
    SET_LHS(type_res.index, ni);
    SET_RHS(NODE_INDEX_NONE, ni);

    return NODE_SUCC(ni);
}

// name: type
// token= name
// lhs= type
// rhs= default value
static NodeRes parse_data_field() {
    const NodeRes typed_ident_res= parse_typed_identifier();
    PROP_FAIL(typed_ident_res);

    SET_TYPE(NT_DATA_FIELD, typed_ident_res.index);

    if (EXPECT(TT_OPERATOR_ASSIGN)) {
        NodeRes default_value_res= parse_expr(false);
        PROP_FAIL(default_value_res)

        SET_RHS(default_value_res.index, typed_ident_res.index);
    }

    return NODE_SUCC(typed_ident_res.index);
}

static NodeRes parse_data_param_fields() {
    Token paren_tok;
    if (!expect(TT_LPAREN, &paren_tok)) {
        return unexpected(TT_LPAREN, current(), "Data parameterised types must be followed by a paren that contains the field entries.\n");
    }

    size_t extra_start= NODE_INDEX_NONE;
    size_t elem_idx= 0;

    if (EXPECT(TT_LPAREN)) goto parse_data_fields_no_fields;
    do {
        const NodeRes field_res= parse_typed_identifier();
        PROP_FAIL(field_res);

        const size_t extra_index= NodeIndex_array_add(&extra_array, field_res.index);
        if (elem_idx == 0) extra_start= extra_index;
        assert(extra_index == (extra_start + elem_idx));
        elem_idx++;

        if (check(TT_EOTS)) {
            return unexpected(TT_RPAREN, current(), "Data parameterised field list must end with right brace '}'.\n");
        }
    } while (EXPECT(TT_COMMA));

    parse_data_fields_no_fields:;

    const NodeIndex ni= node_create(NT_DATA_PARAMETERISED_FIELDS, paren_tok);

    SET_LHS(extra_start, ni);
    SET_RHS(extra_start + elem_idx, ni);

    return NODE_SUCC(ni);
}

static NodeRes parse_data_variant() {
    Token ident_tok;
    if (!expect(TT_IDENTIFIER, &ident_tok)) {
        assert(false && "Attempting to parse data variant without starting with idenfitier.\n");
    }

    NodeIndexArray temp= NodeIndex_array_construct(4);
    if (EXPECT(TT_LPAREN)) {
        if (EXPECT(TT_RPAREN)) {
            goto parse_data_variant_no_types;
        }

        do {
            const NodeRes elem_res= parse_typed_identifier();
            PROP_FAIL(elem_res)

            NodeIndex_array_add(&temp, elem_res.index);
        } while (EXPECT(TT_COMMA));

        if (!EXPECT(TT_RPAREN)) {
            return unexpected(TT_RPAREN, current(), "Data variants that contain types i.e. X(a: i4, b: i4) must end with a right paren.\n");
        }
    }

parse_data_variant_no_types:;
    NodeIndex default_value_index= NODE_INDEX_NONE;
    if (EXPECT(TT_OPERATOR_ASSIGN)) {
        NodeRes default_value_res= parse_expr(false);
        PROP_FAIL(default_value_res);

        default_value_index= default_value_res.index;
    }

    const NodeIndex ni= node_create(NT_DATA_VARIANT, ident_tok);
    const NodeIndex extra_start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    // adding in the default value
    NodeIndex_array_add(&extra_array, default_value_index);
    const NodeIndex extra_end= extra_start + temp.pos;
    SET_LHS(extra_start, ni);
    SET_RHS(extra_end, ni);

    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(ni);
}

static NodeRes parse_data_fields() {
    Token brace_tok;
    if (!expect(TT_LBRACE, &brace_tok)) {
        return unexpected(TT_LBRACE, current(), "Data types must be followed by a brace that contains the field entries.\n");
    }

    eat_all_delimiters();
    
    NodeIndexArray temp= NodeIndex_array_construct(4);
    
    if (EXPECT(TT_RBRACE)) goto parse_data_fields_no_fields;
    do {
        if (peek().type != TT_COLON) break;

        const NodeRes field_res= parse_data_field();
        PROP_FAIL(field_res);
        
        NodeIndex_array_add(&temp, field_res.index);

        if (check(TT_EOTS)) {
            return unexpected(TT_RBRACE, current(), "Data field list must end with right brace '}'.\n");
        }

        eat_all_delimiters();
    } while (!EXPECT(TT_RBRACE));

    eat_all_delimiters();
    if (EXPECT(TT_RBRACE)) goto parse_data_fields_no_variants;

    do {
        const NodeRes variant_res= parse_data_variant();
        PROP_FAIL(variant_res);

        NodeIndex_array_add(&temp, variant_res.index);

        if (check(TT_EOTS)) {
            return unexpected(TT_RBRACE, current(), "Data field list must end with right brace '}'.\n");
        }

        eat_all_delimiters();
    } while (!EXPECT(TT_RBRACE));
    
parse_data_fields_no_fields:
parse_data_fields_no_variants:;
    const NodeIndex extra_start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    
    const NodeIndex ni= node_create(NT_DATA_FIELDS, brace_tok);

    SET_LHS(extra_start, ni);
    SET_RHS(extra_start + temp.pos - 1, ni);

    NodeIndex_array_destroy(&temp);
    
    return NODE_SUCC(ni);
}

// token= data,
// lhs= index to parameterised fields,
// rhs= index to the fields
static NodeRes parse_type_data() {
    const Token data_tok= consume();
    
    const NodeIndex ni= node_create(NT_TYPE_DATA, data_tok);
    
    NodeIndex param_fields= NODE_INDEX_NONE;
    if (check(TT_LPAREN)) {
        const NodeRes param_fields_res= parse_data_param_fields();
        PROP_FAIL(param_fields_res);
        
        param_fields= param_fields_res.index;
    }
    
    const NodeRes fields_res= parse_data_fields();
    PROP_FAIL(fields_res);
    
    const NodeIndex fields= fields_res.index;
    SET_LHS(param_fields, ni);
    SET_RHS(fields, ni);
    
    return NODE_SUCC(ni);
}

static NodeRes parse_type_const() {
    // this can be
    // const <type>
    // or
    // const
    // to allow for adding const to an inferred type
    const Token const_tok= consume();
    assert(const_tok.type == TT_KEYWORD_CONST && "parsing const, but did not get const keyword.\n");

    const NodeRes type_res= parse_type();
    PROP_FAIL(type_res)

    const NodeIndex ni= node_create(NT_TYPE_CONST, const_tok);
    SET_LHS(type_res.index, ni);

    return NODE_SUCC(ni);
}

bool is_valid_type_start(const TokenType type) {
    if (type > TT_INTERNAL_TYPE_START && type < TT_INTERNAL_TYPE_END) return true;

    switch (type) {
        case TT_OPERATOR_AMBIGUOUS_MORE:
        case TT_KEYWORD_DATA:
        case TT_KEYWORD_CONST:
        case TT_IDENTIFIER: return true;
        default: return false;
    }
}

// todo: maybe could store the types in a row, with the type ending on a base type tag
NodeRes parse_type() {
    // >        -X- pointer prefix
    // n8       -X- base type
    // Buffer   -X- identifier
    // '        -X- mark
    // ?        -X- nullable     -- postfix
    // !        -X- errorable    -- infix, error to the left, ret type to the right
    // []       -X- arrays
    // ()       -X- lambda header
    // data     -X- data type
    // const    - -
    // Errors!>i4

    NodeRes res;
    if (check(TT_KEYWORD_CONST)) {
        res= parse_type_const();
        goto parse_type_postfix;
    }

    if (check(TT_OPERATOR_AMBIGUOUS_MORE)) {
        res= parse_type_pointer();
        goto parse_type_postfix;
    }

    if (peek().type == TT_OPERATOR_AMBIGUOUS_EXCLAMATION || check(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
        res= parse_type_errorable();
        goto parse_type_postfix;
    }
    
    if (check(TT_KEYWORD_DATA)) {
        res= parse_type_data();
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
            case TT_OPERATOR_AMBIGUOUS_DOT: {
                // [..]
                // lhs= inner type
                consume();

                if (!EXPECT(TT_OPERATOR_AMBIGUOUS_DOT)) return unexpected(
                    TT_OPERATOR_AMBIGUOUS_DOT, current(),
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
            case TT_OPERATOR_BITWISE_NOT: {
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
                const NodeRes expr_res= parse_expr(false);
                PROP_FAIL(expr_res)

                array_node_index= node_create(NT_TYPE_ARRAY_STATIC, tok);
                SET_RHS(expr_res.index, array_node_index);

                break;
            }
        }

        if (!EXPECT(TT_RBRACKET)) {
            return unexpected(TT_RBRACKET, current(), "Array type syntax must have a closing bracket, e.g. [..], did not find it.\n");
        }

        const NodeRes inner_type_res= parse_type();
        PROP_FAIL(inner_type_res);

        SET_LHS(inner_type_res.index, array_node_index);

        res= NODE_SUCC(array_node_index);
        goto parse_type_postfix;
    }

    if (check(TT_LPAREN)) {
        res= parse_lambda_header();
        goto parse_type_postfix;
    }

    parse_error("Unable to parse type. Starter token is not valid\n");
    PRINT_TOKEN(current());
    return NODE_FAIL(EC_FAIL);

parse_type_postfix:
    PROP_FAIL(res)

    // check for marks and nullables
    while (true) {
        if (expect(TT_OPERATOR_AMBIGUOUS_QUESTION, &tok)) {
            NodeIndex node= node_create(NT_TYPE_NULLABLE, tok);
            SET_LHS(res.index, node);
            res.index= node;
        } else if (expect(TT_OPERATOR_CARROT, &tok)) {
            NodeIndex node= node_create(NT_TYPE_MARKED, tok);
            SET_LHS(res.index, node);
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
        SET_LHS(type_res.index, param_index);
        SET_RHS(NODE_INDEX_NONE, param_index);

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
    const NodeIndex extra_end= extra_start + params.pos - 1;

    SET_LHS(extra_start, header_index);
    SET_RHS(extra_end, header_index);

    return NODE_SUCC(header_index);
}

NodeRes parse_import_statement() {
    assert(false);
}

NodeRes parse_entry_statement() {
    assert(false);
}

static NodeRes parse_top_level_statement() {
    eat_all_delimiters();

    Token t= current();

    switch (t.type) {
        case TT_IDENTIFIER: return parse_variable_declaration();
        case TT_KEYWORD_IMPORT: return parse_import_statement();
        case TT_KEYWORD_ENTRY: return parse_entry_statement();
        default: {
            parse_error("Expected variable declaration or import statement at top level of program. Found: ");
            PRINT_TOKEN(t);
            newline();
            return NODE_FAIL(EC_FAIL);
        }
    }
}

static NodeRes parse_arg() {
    return parse_expr(true);
}

static NodeRes parse_expr(bool allow_trailing_parens) {
    return shunt(allow_trailing_parens);
}

static NodeRes parse_function_call() {
    const Token ident= consume();
    assert(ident.type == TT_IDENTIFIER && "parsing function call but didn't get identifier first.\n");

    assert(EXPECT(TT_LPAREN) && "parsing function call but did not get LPAREN after ident.\n");

    NodeIndexArray temp= NodeIndex_array_construct(4);
    if (EXPECT(TT_RPAREN)) goto parse_function_call_no_args;
    do {
        const NodeRes arg_res= parse_arg();
        PROP_FAIL(arg_res);

        NodeIndex_array_add(&temp, arg_res.index);

        if (check(TT_EOTS)) return unexpected(TT_RPAREN, current(), "Parsing function call, received end of token stream before closing paren.\n");
    } while (EXPECT(TT_COMMA));

parse_function_call_no_args:;
    const NodeIndex ni= node_create(NT_EXPR_FUNC_CALL, ident);
    const size_t extra_start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    const size_t extra_end= extra_start + temp.pos - 1;

    SET_LHS(extra_start, ni);
    SET_RHS(extra_end, ni);

    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(ni);
}

static NodeRes parse_array_index() {
    // <ident>[<expr>]
    const Token ident= consume();

    assert(ident.type == TT_IDENTIFIER && "parsing array index, first token is not ident.\n");
    assert(EXPECT(TT_LBRACKET) && "parsing array index, did not find LBRACKET after ident.\n");

    const NodeRes expr_res= parse_expr(false);
    PROP_FAIL(expr_res)

    if (!EXPECT(TT_RBRACKET)) {
        return unexpected(TT_RBRACKET, current(), "Array index must have right bracket after expression. E.g. a[1]\n");
    }

    const NodeIndex ni= node_create(NT_EXPR_ARRAY_INDEX, ident);
    SET_LHS(expr_res.index, ni);
    SET_RHS(NODE_INDEX_NONE, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_identifier() {
    // variable
    // function call
    // an array index x[]
    const TokenType peeked_type= peek().type;

    if (peeked_type == TT_LPAREN) return parse_function_call();
    if (peeked_type == TT_LBRACKET) return parse_array_index();

    const Token ident= consume();
    const NodeIndex ni= node_create(NT_EXPR_VARIABLE, ident);

    return NODE_SUCC(ni);
}

NodeRes parse_ret() {
    const Token ret_token= consume();
    assert(ret_token.type == TT_KEYWORD_RET && "parsing return but first token is not return.\n");

    NodeIndex result= NODE_INDEX_NONE;
    if (is_valid_expr_start(current().type)) {
        const NodeRes result_res= parse_expr(true);
        PROP_FAIL(result_res)

        result= result_res.index;
    }

    const NodeIndex ni= node_create(NT_EXPR_RETURN, ret_token);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_break() {
    const Token break_tok= consume();
    assert(break_tok.type == TT_KEYWORD_BREAK && "parsing break but first token is not break.\n");

    NodeIndex result= NODE_INDEX_NONE;
    if (is_valid_expr_start(current().type)) {
        const NodeRes result_res= parse_expr(true);
        PROP_FAIL(result_res)

        result= result_res.index;
    }

    const NodeIndex ni= node_create(NT_EXPR_BREAK, break_tok);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_continue() {
    const Token continue_tok= consume();
    assert(continue_tok.type == TT_KEYWORD_CONTINUE && "parsing continue but first token is not continue.\n");

    const NodeIndex ni= node_create(NT_EXPR_CONTINUE, continue_tok);
    return NODE_SUCC(ni);
}

NodeRes parse_for() {
    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_while() {
    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_do() {
    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_if() {
    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_brace() {
    return NODE_FAIL(EC_FAIL);
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

NodeIndex node_create(const NodeType type, const Token token) {
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

static VarDeclNode var_decl_node_get(NodeIndex index) {
    if (index >= node_array.types.pos) assert(false);

    return (VarDeclNode) {
        .token= Token_array_get(&node_array.tokens, index),
        .type= NodeIndex_array_get(&node_array.lhs, index),
        .value= NodeIndex_array_get(&node_array.rhs, index)
    };
}

static inline bool is_delimiter(const TokenType type) {
    return type == TT_DELIMITER || type == TT_NEWLINE;
}

void eat_all_delimiters() {
    while (is_delimiter(current().type)) consume();
}

Token current() {
    if (t_pos >= tokens.pos) return (Token) {
        .type= TT_EOTS,
        .buffer_index= -1
    };

    return Token_array_get(&tokens, t_pos);
}

Token consume() {
    const Token res= current();
    t_pos++;
    return res;
}

Token peek() {
    return peer(1);
}

Token peer(size_t offset) {
    if (t_pos + offset >= tokens.pos) return (Token) {
        .type= TT_EOTS,
        .buffer_index= -1
    };

    return Token_array_get(&tokens, t_pos + offset);
}

bool check(const TokenType type) {
    return (current().type == type);
}

bool expect(const TokenType type, Token* out) {
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

inline bool between(TokenType type, TokenType start_exclusive, TokenType end_exclusive) {
    return (start_exclusive < type) && (type < end_exclusive);
}

#undef EXPECT
#undef EXPECTED_NODE_COUNT
