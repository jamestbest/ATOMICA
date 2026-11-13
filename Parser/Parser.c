//
// Created by jamescoward on 23/10/2026.
//

#include <stdbool.h>

#include "Parser.h"

#include "ParserInternal.h"
#include "SharedParsing.h"
#include "Lexer/Token.h"
#include "Parserr/Parserr.h"
#include "shared/String.h"
#include "Shunt/ShuntingYard.h"

// todo: _add functions return u64, but we store u32s
// todo: zero init node_array or write INDEX_NONE (don't do that)
// todo: eat delimiters in braces

__thread MyTokenSource source;

__thread NodeArray node_array;
// if lhs or rhs are NODE_INDEX_NONE then there is no data in the extra_array
__thread NodeIndexArray extra_array;

#define EXPECTED_NODE_COUNT 100
// _Static_assert(EXPECTED_NODE_COUNT > 1, "");

#define MAX_ENTRY_POINTS 10
NodeIndex entry_points[MAX_ENTRY_POINTS];
size_t entry_point_pos;

NodeRes parse(MyTokenSource source_) {
    source= source_;
    source.current_idx= source.start_idx;

    setup_node_array();
    extra_array= NodeIndex_array_construct(EXPECTED_NODE_COUNT);
    extra_array.pos++;

    const NodeIndex root= node_create(NT_ROOT, (Token){0});

    NodeIndexArray temp= NodeIndex_array_construct(10);
    while (source.current_idx <= source.end_idx) {
        eat_all_delimiters();

        if (check(TT_EOTS)) break;

        const NodeRes res= parse_top_level_statement();
        if (res.errcode != EC_SUCCESS) return NODE_FAIL(res.errcode);
        print_root_node(res.index);

        const NodeIndex ni= res.index;
        NodeIndex_array_add(&temp, ni);
    }

    NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(root);
}

static NodeRes parse_top_level_statement() {
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

// can either be just the header, or the header + body
static NodeRes parse_lambda() {
    if (!check(TT_LPAREN)) {
        assert(false && "Attempted to parse lambda but did not get lparen as first token.\n");
    }

    const NodeRes header_res= parse_lambda_header();
    PROP_FAIL(header_res);

    if (check(TT_LBRACE)) {
        const NodeRes body_res= parse_compound_statement();
        PROP_FAIL(body_res)

        const NodeIndex ni= node_create(NT_LAMBDA, GET_TOKEN(header_res.index));
        SET_LHS(header_res.index, ni);
        SET_RHS(body_res.index, ni);

        return NODE_SUCC(ni);
    }

    return header_res;
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

    // special cases
    // import & lambda
    if (check(TT_KEYWORD_IMPORT)) {
        const NodeRes import_res= parse_import_statement();
        PROP_FAIL(import_res)

        value= import_res.index;
    } else if (peer(2).type == TT_COLON) {
        const NodeRes lambda_res= parse_lambda();
        PROP_FAIL(lambda_res);

        value= lambda_res.index;
    } else {
        const NodeRes value_res= parse_expr(false);
        PROP_FAIL(value_res);

        value= value_res.index;
    }

parse_vd_no_value:;
    const NodeIndex ni= node_create(node_type, ident);
    SET_LHS(type, ni);
    SET_RHS(value, ni);

    return NODE_SUCC(ni);
}

static NodeRes parse_type_errorable() {
    NodeIndex error_types= NODE_INDEX_NONE;

    NodeIndex ni;

    if (!check(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
        Token identifier;
        if (!expect(TT_IDENTIFIER, &identifier)) {
            return unexpected(TT_IDENTIFIER, current(), "Currently error return types must be identifiers.\n");
        }

        error_types= TOKEN_AS_NODE_INDEX(identifier);

        if (!check(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
            return unexpected(TT_OPERATOR_AMBIGUOUS_EXCLAMATION, current(), "<<DEV>> How did we get here, we are parsing an errorable, but did not get an exclamation after identifier");
        }

        ni= node_create(NT_TYPE_ERRORABLE, consume());
    } else {
        ni= node_create(NT_TYPE_ERRORABLE, consume());
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

    const NodeIndex ni= node_create(NT_TYPE_POINTER, ptr);
    const NodeIndex wrapped_type= parse_type_append_postfixes(ni);

    NodeRes inner_type_res= parse_type();
    PROP_FAIL(inner_type_res)

    SET_LHS(inner_type_res.index, ni);

    return NODE_SUCC(wrapped_type);
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

bool is_valid_type_start(const MyTokenType type) {
    if (type > TT_INTERNAL_TYPE_START && type < TT_INTERNAL_TYPE_END) return true;

    switch (type) {
        case TT_OPERATOR_AMBIGUOUS_MORE:
        case TT_KEYWORD_DATA:
        case TT_KEYWORD_CONST:
        case TT_IDENTIFIER: return true;
        default: return false;
    }
}

// check for ' and ? on a type
static NodeIndex parse_type_append_postfixes(NodeIndex res) {
    Token tok;

    while (true) {
        if (expect(TT_OPERATOR_AMBIGUOUS_QUESTION, &tok)) {
            NodeIndex node= node_create(NT_TYPE_NULLABLE, tok);
            SET_LHS(res, node);
            res= node;
        } else if (expect(TT_OPERATOR_POSTFIX_CARROT, &tok)) {
            NodeIndex node= node_create(NT_TYPE_MARKED, tok);
            SET_LHS(res, node);
            res= node;
        } else break;
    }

    return res;
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
    // const    -X-
    // Errors!>i4

    NodeRes res;
    if (check(TT_KEYWORD_CONST)) {
        res= parse_type_const();
        goto parse_type_end;
    }

    if (check(TT_OPERATOR_AMBIGUOUS_MORE)) {
        res= parse_type_pointer();
        goto parse_type_end;
    }

    if (peek().type == TT_OPERATOR_AMBIGUOUS_EXCLAMATION || check(TT_OPERATOR_AMBIGUOUS_EXCLAMATION)) {
        res= parse_type_errorable();
        goto parse_type_end;
    }
    
    if (check(TT_KEYWORD_DATA)) {
        res= parse_type_data();
        goto parse_type_end;
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

        const NodeIndex wrapped_type= parse_type_append_postfixes(array_node_index);

        const NodeRes inner_type_res= parse_type();
        PROP_FAIL(inner_type_res);

        SET_LHS(inner_type_res.index, array_node_index);

        res= NODE_SUCC(wrapped_type);
        goto parse_type_end;
    }

    if (check(TT_LPAREN)) {
        res= parse_lambda_header();
        goto parse_type_end;
    }

    parse_error("Unable to parse type. Starter token is not valid\n");
    PRINT_TOKEN(current());
    return NODE_FAIL(EC_FAIL);

parse_type_postfix:
    PROP_FAIL(res)

    res.index= parse_type_append_postfixes(res.index);
    return res;

parse_type_end:
    PROP_FAIL(res)

    return res;
}

NodeRes parse_array_literal() {
    Token lbracket;
    if (!expect(TT_LBRACKET, &lbracket)) {
        assert(false && "Attempted to parse array literal, but did not start with lbracket.\n");
    }

    NodeIndexArray temp= NodeIndex_array_construct(3);

    if (EXPECT(TT_RBRACKET)) goto parse_array_literal_no_elements;

    do {
        const NodeRes elem_res= parse_expr(false);
        PROP_FAIL(elem_res);

        NodeIndex_array_add(&temp, elem_res.index);
    } while (EXPECT(TT_COMMA));

    if (!EXPECT(TT_RBRACKET)) {
        return unexpected(TT_RBRACKET, current(), "Array literal must be enclosed in brackets i.e. [], did not find right bracket.\n");
    }

parse_array_literal_no_elements:
    const NodeIndex start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    const NodeIndex end= start + temp.pos - 1;

    const NodeIndex ni= node_create(NT_EXPR_LITERAL_ARRAY, lbracket);
    SET_LHS(start, ni);
    SET_RHS(end, ni);

    return NODE_SUCC(ni);
}

// {}
static NodeRes parse_code_literal() {
    Token brace;
    if (!expect(TT_LBRACE, &brace)) {
        assert(false && "Attempted to parse code literal but did not start with LBRACE.\n");
    }

    MyTokenSource new_source= {
        .data= source.data,
        .start_idx= source.current_idx,
        .end_idx= source.current_idx
    };

    const NodeIndex ni= node_create(NT_EXPR_CODE_LITERAL, brace);

    size_t lbrace_count= 1;
    while (true) {
        const MyTokenType type= current().type;
        if (type == TT_LBRACE) lbrace_count++;
        if (type == TT_RBRACE) lbrace_count--;
        if (type == TT_EOTS) break;

        if (lbrace_count == 0) break;

        consume();
    }
    new_source.end_idx= source.current_idx - 1;

    if (!EXPECT(TT_RBRACE)) {
        return unexpected(TT_RBRACE, current(), "Code literals must be enclosed in ${}\n");
    }

    const NodeIndex start= array_add_dyn((Array*)&extra_array, &new_source, sizeof(MyTokenSource));
    SET_LHS(start, ni);
    SET_RHS(extra_array.pos - 1, ni);

    return NODE_SUCC(ni);
}

// this is not an identifier it is the string name of the identifier
static NodeRes parse_identifier_literal() {
    Token ident;
    if (!expect(TT_IDENTIFIER, &ident)) {
        assert(false && "Attempted to parse identifier literal ($<ident>), but did not start with identifier.\n");
    }

    const NodeIndex ni= node_create(NT_IDENTIFIER_LITERAL, ident);

    return NODE_SUCC(ni);
}

// ${} -- code literal
// $<ident> -- identifier
// $"" -- maybe: string interp
NodeRes parse_dollar() {
    Token dollar;
    if (!expect(TT_DOLLAR, &dollar)) {
        assert(false && "Attempted to parse dollar, but first token was not $.\n");
    }

    if (check(TT_LBRACE)) {
        return parse_code_literal();
    }

    if (check(TT_IDENTIFIER)) {
        return parse_identifier_literal();
    }

    return parse_error("Invalid token after dollar. Only accept code literals atm (${}).\n");
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

    if (check(TT_RPAREN)) goto parse_lambda_header_no_params;

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

    if (!EXPECT(TT_COLON)) {
        return unexpected(TT_COLON, current(), "Lambda header requires return type after parameter list, specified by ':<type>' e.g. (): i4\n");
    }

    const NodeRes return_type_res= parse_type();
    PROP_FAIL(return_type_res);

    const NodeIndex header_index= node_create(NT_LAMBDA_HEADER, paren_token);
    const NodeIndex extra_start= NodeIndex_array_add_bulk(&extra_array, params.data, params.pos);
    NodeIndex_array_add(&extra_array, return_type_res.index);
    const NodeIndex extra_end= extra_start + params.pos;

    SET_LHS(extra_start, header_index);
    SET_RHS(extra_end, header_index);

    return NODE_SUCC(header_index);
}

static NodeRes parse_import_statement() {
    Token import_kw;
    if (!expect(TT_KEYWORD_IMPORT, &import_kw)) {
        assert(false && "Attempted to parse import statement but did not start with import.\n");
    }

    Token file_string;
    if (!expect(TT_LIT_STRING, &file_string)) {
        return unexpected(TT_LIT_STRING, current(), "Import statements must have a string literal after, stating the file to import.\n");
    }

    const NodeIndex ni= node_create(NT_IMPORT_STATEMENT, import_kw);
    SET_LHS(TOKEN_AS_NODE_INDEX(file_string), ni);

    return NODE_SUCC(ni);
}

static NodeRes parse_entry_statement() {
    consume();

    const NodeRes vd_res= parse_variable_declaration();
    PROP_FAIL(vd_res);

    entry_points[entry_point_pos++]= vd_res.index;
    return vd_res;
}

static NodeRes parse_statement() {
    // can either be
    //  - variable declaration
    //  - expression

    if (peek().type == TT_COLON) {
        return parse_variable_declaration();
    }
    return parse_expr(false);
}

NodeRes parse_compound_statement() {
    Token lbrace;
    if (!expect(TT_LBRACE, &lbrace)) {
        assert(false && "Attempted to parse compound statement, but does not start with lbrace.\n");
    }

    NodeIndexArray temp= NodeIndex_array_construct(6);

    eat_all_delimiters();
    while (!EXPECT(TT_RBRACE)) {
        const NodeRes statement_res= parse_statement();
        PROP_FAIL(statement_res);

        eat_all_delimiters();

        NodeIndex_array_add(&temp, statement_res.index);
    }

    const NodeIndex start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    const NodeIndex end= start + temp.pos - 1;

    const NodeIndex ni= node_create(NT_COMPOUND_STATEMENT, lbrace);

    SET_LHS(start, ni);
    SET_RHS(end, ni);

    NodeIndex_array_destroy(&temp);

    return NODE_SUCC(ni);
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

    if (!EXPECT(TT_RPAREN)) return unexpected(TT_RPAREN, current(), "Function call arguments must be enclosed in parenthesis, got left but not right.\n");

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
    const MyTokenType peeked_type= peek().type;

    if (peeked_type == TT_LPAREN) return parse_function_call();
    if (peeked_type == TT_LBRACKET) return parse_array_index();

    const Token ident= consume();
    const NodeIndex ni= node_create(NT_EXPR_VARIABLE, ident);

    return NODE_SUCC(ni);
}

NodeRes parse_ret() {
    const Token ret_token= consume();
    assert(ret_token.type == TT_KEYWORD_RET && "parsing return but first token is not return.\n");

    bool is_error_return= EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION);

    NodeIndex result= NODE_INDEX_NONE;
    if (is_valid_expr_start(current().type)) {
        const NodeRes result_res= parse_expr(true);
        PROP_FAIL(result_res)

        result= result_res.index;
    }

    const NodeType type= is_error_return ? NT_EXPR_RETURN_ERROR : NT_EXPR_RETURN;
    const NodeIndex ni= node_create(type, ret_token);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_yield() {
    const Token yield_token= consume();
    assert(yield_token.type == TT_KEYWORD_YIELD && "parsing yield but first token is not yield.\n");

    const bool is_error_yield= EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION);

    NodeIndex result= NODE_INDEX_NONE;
    if (is_valid_expr_start(current().type)) {
        const NodeRes result_res= parse_expr(true);
        PROP_FAIL(result_res)

        result= result_res.index;
    }

    const NodeType type= is_error_yield ? NT_EXPR_YIELD_ERROR : NT_EXPR_YIELD;
    const NodeIndex ni= node_create(type, yield_token);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_defer() {
    const Token defer_token= consume();
    assert(defer_token.type == TT_KEYWORD_DEFER && "parsing defer but first token is not defer.\n");

    const bool is_error_defer= EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION);

    const NodeRes result_res= parse_statement();
    PROP_FAIL(result_res)

    const NodeIndex result= result_res.index;

    const NodeType type= is_error_defer ? NT_EXPR_DEFER_ERROR : NT_EXPR_DEFER;
    const NodeIndex ni= node_create(type, defer_token);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_break() {
    const Token break_tok= consume();
    assert(break_tok.type == TT_KEYWORD_BREAK && "parsing break but first token is not break.\n");

    const bool is_error_return= EXPECT(TT_OPERATOR_AMBIGUOUS_EXCLAMATION);

    NodeIndex result= NODE_INDEX_NONE;
    if (is_valid_expr_start(current().type)) {
        const NodeRes result_res= parse_expr(true);
        PROP_FAIL(result_res)

        result= result_res.index;
    }

    const NodeType type= is_error_return ? NT_EXPR_BREAK_ERROR : NT_EXPR_BREAK;
    const NodeIndex ni= node_create(type, break_tok);
    SET_LHS(result, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_continue() {
    const Token continue_tok= consume();
    assert(continue_tok.type == TT_KEYWORD_CONTINUE && "parsing continue but first token is not continue.\n");

    const NodeIndex ni= node_create(NT_EXPR_CONTINUE, continue_tok);
    return NODE_SUCC(ni);
}

NodeRes parse_for_each() {
    Token for_token;
    if (!expect(TT_KEYWORD_FOR, &for_token)) {
        assert(false && "Attempted to parse for-each expression, but first token is not for.\n");
    }

    if (!EXPECT(TT_LPAREN)) {
        return unexpected(TT_LPAREN, current(), "For-each expressions require being wrapped in parenthesis, e.g. for (x) {}.\n");
    }

    NodeIndexArray temp= NodeIndex_array_construct(4);

    if (peek().type == TT_COLON) {
        Token elem_token;
        if (!expect(TT_IDENTIFIER, &elem_token)) {
            return unexpected(TT_IDENTIFIER, current(), "For each expression that uses element in array format requires identifier:array. Didn't get element identifier.\n");
        }

        NodeIndex_array_add(&temp, TOKEN_AS_NODE_INDEX(elem_token));

        if (!EXPECT(TT_COLON)) assert(false && "Attempted to parse for each with elem:array syntax but did not get colon.\n");

        // todo: could support; for x: [1,2,3]
        Token array_token;
        if (!expect(TT_IDENTIFIER, &array_token)) {
            return unexpected(TT_IDENTIFIER, current(), "For each expression that uses element in array format requires identifier:array. Didn't get array identifier.\n");
        }

        NodeIndex_array_add(&temp, TOKEN_AS_NODE_INDEX(array_token));
    } else {
        // for x
        const NodeRes loop_res= parse_expr(true);
        PROP_FAIL(loop_res);

        NodeIndex_array_add(&temp, loop_res.index);
        NodeIndex_array_add(&temp, NODE_INDEX_NONE);
    }

    if (EXPECT(TT_KEYWORD_WITH)) {
        Token with_ident_token;
        if (!expect(TT_IDENTIFIER, &with_ident_token)) {
            return unexpected(TT_IDENTIFIER, current(), "With requires identifier to give name to loop index.\n");
        }

        NodeIndex_array_add(&temp, TOKEN_AS_NODE_INDEX(with_ident_token));
    } else {
        NodeIndex_array_add(&temp, NODE_INDEX_NONE);
    }

    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(), "For each expressions requires closing right parenthesis. E.g. for (x) {}\n");
    }

    const NodeRes output_res= parse_expr(false);
    PROP_FAIL(output_res);

    NodeIndex_array_add(&temp, output_res.index);

    const NodeIndex ni= node_create(NT_FOR_EACH_EXPR, for_token);
    const NodeIndex start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    const NodeIndex end= start + temp.pos - 1;

    SET_LHS(start, ni);
    SET_RHS(end, ni);

    return NODE_SUCC(ni);
}

static NodeRes parse_for_cstyle() {
    Token for_token;
    if (!expect(TT_KEYWORD_FOR, &for_token)) {
        assert(false && "Attempted to parse for (cstyle) expression, but first token is not for.\n");
    }

    if (!EXPECT(TT_LPAREN)) {
        return unexpected(TT_LPAREN, current(), "For (cstyle) expressions require being wrapped in parenthesis, e.g. for (x: i4= 0 to x < 10) {}.\n");
    }

    NodeIndexArray temp= NodeIndex_array_construct(4);

    // INIT CHECK
    if (check(TT_KEYWORD_TO)) {
        // no init
        NodeIndex_array_add(&temp, NODE_INDEX_NONE); // no init
        goto parse_for_cstyle_no_init;
    }

    if (check(TT_KEYWORD_DO)) {
        // no init and no cond
        NodeIndex_array_add(&temp, NODE_INDEX_NONE); // no init
        NodeIndex_array_add(&temp, NODE_INDEX_NONE); // no cond
        goto parse_for_cstyle_no_cond;
    }

    if (is_assign_operator(peek().type)) {
        const NodeRes va_res= parse_expr(false);
        PROP_FAIL(va_res);

        NodeIndex_array_add(&temp, va_res.index);
    } else if (peek().type == TT_COLON) {
        const NodeRes vd_res= parse_variable_declaration();
        PROP_FAIL(vd_res);

        NodeIndex_array_add(&temp, vd_res.index);
    }

parse_for_cstyle_no_init:

    // COND CHECK
    if (EXPECT(TT_KEYWORD_TO)) {
        const NodeRes cond_res= parse_expr(false);
        PROP_FAIL(cond_res);

        NodeIndex_array_add(&temp, cond_res.index);
    } else {
        NodeIndex_array_add(&temp, NODE_INDEX_NONE);
    }

parse_for_cstyle_no_cond:
    // LOOP CHECK
    if (EXPECT(TT_KEYWORD_DO)) {
        const NodeRes cond_res= parse_expr(false);
        PROP_FAIL(cond_res);

        NodeIndex_array_add(&temp, cond_res.index);
    } else {
        NodeIndex_array_add(&temp, NODE_INDEX_NONE);
    }

    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(), "For (cstyle) expressions requires closing right parenthesis. E.g. for (x:i4= 0 to x < 10) {}\n");
    }

    const NodeRes output_res= parse_expr(false);
    PROP_FAIL(output_res);

    NodeIndex_array_add(&temp, output_res.index);

    const NodeIndex start= NodeIndex_array_add_bulk(&extra_array, temp.data, temp.pos);
    const NodeIndex end= start + temp.pos - 1;

    const NodeIndex ni= node_create(NT_FOR_EXPR, for_token);
    SET_LHS(start, ni);
    SET_RHS(end, ni);

    return NODE_SUCC(ni);
}

NodeRes parse_for() {
    const size_t t_pos_before= source.current_idx;

    Token for_token;
    if (!expect(TT_KEYWORD_FOR, &for_token)) {
        assert(false && "Attempted to parse for expression, but first token is not for.\n");
    }

    NodeType type= NT_FOR_EACH_EXPR;

    const Token p2= peek();
    // todo: may cause issues when impl. metaprogramming
    //  which inserts tokens before the for loop
    // we need to find the type of for loop i.e. c-style or for-each
    // c-style could be for ;;
    // for each doesn't allow for vd with initialisation e.g. x:A=0, just x:A
    // maybe restore the token pointer after parsing vd? to split this into two
    //  functions? this may cause issues with the metaprogramming
    // for x #insert ${ : i4 }

    // funcA :: () { #insert #run funcB() }
    // funcB :: () {}
    // funcC :: () { for i: #insert #run funcA() }

    // todo: crude way to check which type of for loop it is
    // lookhead(1)
    if (is_assign_operator(p2.type)) {
        const NodeRes va_res= parse_expr(false);
        PROP_FAIL(va_res);

        if (EXPECT(TT_KEYWORD_TO)) {
            // here we are the for loop
            type= NT_FOR_EXPR;
        } else {
            // here we are a foreach loop
            type= NT_FOR_EACH_EXPR;
        }
    } else if (p2.type == TT_COLON) {
        const NodeRes vd_res= parse_variable_declaration();
        PROP_FAIL(vd_res);

        if (EXPECT(TT_KEYWORD_TO)) {
            // here we are the for loop
            type= NT_FOR_EXPR;
        } else {
            // here we are a foreach loop
            type= NT_FOR_EACH_EXPR;
        }
    }

    source.current_idx= t_pos_before;

    if (type == NT_FOR_EACH_EXPR) return parse_for_each();
    if (type == NT_FOR_EXPR) return parse_for_cstyle();

    assert(false);
}

NodeRes parse_while() {
    Token while_token;
    if (!expect(TT_KEYWORD_WHILE, &while_token)) {
        assert(false && "Attempted to parse while, but the first token is not while.\n");
    }

    if (!EXPECT(TT_LPAREN)) {
        return unexpected(TT_LPAREN, current(),
            "The condition of a while must be wrapped in parenthesis. Sorry :(.\n");
    }

    const NodeRes cond_res= parse_expr(false);
    PROP_FAIL(cond_res);

    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(),
            "The condition of a while must be wrapped in parenthesis, did not find closing paren.\n");
    }

    const NodeRes output_res= parse_expr(false);
    PROP_FAIL(output_res);

    const NodeIndex ni= node_create(NT_WHILE_EXPR, while_token);
    SET_LHS(cond_res.index, ni);
    SET_RHS(output_res.index, ni);

    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_do() {
    Token do_token;
    if (!expect(TT_KEYWORD_DO, &do_token)) {
        assert(false && "Attempted to parse do-while, but the first token is not do.\n");
    }

    const NodeRes output_res= parse_expr(false);
    PROP_FAIL(output_res);

    if (!EXPECT(TT_KEYWORD_WHILE)) {
        return unexpected(TT_KEYWORD_WHILE, current(), "Do while expressions require while condition after output expression. E.g. do {} while x\n");
    }

    if (!EXPECT(TT_LPAREN)) {
        return unexpected(TT_LPAREN, current(),
            "The condition of a do-while must be wrapped in parenthesis. Sorry :(.\n");
    }

    const NodeRes cond_res= parse_expr(false);
    PROP_FAIL(cond_res);

    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(),
            "The condition of a do-while must be wrapped in parenthesis, did not find closing paren.\n");
    }

    const NodeIndex ni= node_create(NT_DO_WHILE_EXPR, do_token);
    SET_LHS(cond_res.index, ni);
    SET_RHS(output_res.index, ni);

    return NODE_FAIL(EC_FAIL);
}

NodeRes parse_if() {
    Token if_token;
    if (!expect(TT_KEYWORD_IF, &if_token)) {
        assert(false && "Attempted to parse if statement, but the first token is not if.\n");
    }

    // if () {}
    if (!EXPECT(TT_LPAREN)) {
        return unexpected(TT_LPAREN, current(),
            "The condition of an if must be wrapped in parenthesis. Sorry :(.\n");
    }

    const NodeRes cond_res= parse_expr(true);
    PROP_FAIL(cond_res);

    if (!EXPECT(TT_RPAREN)) {
        return unexpected(TT_RPAREN, current(),
            "The condition of an if must be wrapped in parenthesis, did not find closing paren.\n");
    }

    const NodeRes output_res= parse_expr(false);
    PROP_FAIL(output_res)

    NodeIndex else_idx= NODE_INDEX_NONE;
    if (check(TT_KEYWORD_ELSE)) {
        const NodeRes else_res= parse_else();
        PROP_FAIL(else_res)

        else_idx= else_res.index;
    }

    NodeIndex to_add[3]= {
        cond_res.index,
        output_res.index,
        else_idx
    };

    const NodeIndex ni= node_create(NT_IF_EXPR, if_token);
    const NodeIndex start= NodeIndex_array_add_bulk(&extra_array,
        to_add, 3
    );

    SET_LHS(start, ni);
    SET_RHS(start + 2, ni);

    return NODE_SUCC(ni);
}

static NodeRes parse_else() {
    Token else_token;
    if (!expect(TT_KEYWORD_ELSE, &else_token)) {
        assert(false && "Attempted to parse else statement, but the first token is not else.\n");
    }

    const NodeRes output_res= parse_statement();
    PROP_FAIL(output_res)

    const NodeIndex ni= node_create(NT_ELSE_EXPR, else_token);
    SET_LHS(output_res.index, ni);

    return NODE_SUCC(ni);
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

static inline bool is_delimiter(const MyTokenType type) {
    return type == TT_DELIMITER || type == TT_NEWLINE;
}

void eat_all_delimiters() {
    while (is_delimiter(current().type)) consume();
}

Token current() {
    if (source.current_idx > source.end_idx) return (Token) {
        .type= TT_EOTS,
        .buffer_index= -1
    };

    return source.data[source.current_idx];
}

Token consume() {
    const Token res= current();
    source.current_idx++;
    return res;
}

Token peek() {
    return peer(1);
}

Token peer(size_t offset) {
    if (source.current_idx + offset > source.end_idx) return (Token) {
        .type= TT_EOTS,
        .buffer_index= -1
    };

    return source.data[source.current_idx + offset];
}

bool check(const MyTokenType type) {
    return (current().type == type);
}

bool expect(const MyTokenType type, Token* out) {
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

inline bool between(MyTokenType type, MyTokenType start_exclusive, MyTokenType end_exclusive) {
    return (start_exclusive < type) && (type < end_exclusive);
}

#undef EXPECT
#undef EXPECTED_NODE_COUNT
