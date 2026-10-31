//
// Created by jamescoward on 25/10/2026.
//

#include "ShuntingYard.h"

#include <stdio.h>
#include <string.h>

#include "Parser/Parser.h"
#include "Parser/SharedParsing.h"
#include "Parser/Parserr/Parserr.h"
#include "shared/String.h"

static uint8_t get_prec(TokenType type);
static bool is_prefix_operator(TokenType type);
static bool is_postfix_operator(TokenType type);
static bool is_unary_operator(TokenType type);
static bool is_assign_operator(TokenType type);
static bool is_right_assoc(TokenType type);
static bool is_binary_operator(TokenType type);
static bool is_operator(TokenType type);

typedef enum {
    // operand like
    EXPECTING_START,

    // just got operator, expecting operand OR LPAREN
    // + 14, + (14), ret, continue, break, {},
    EXPECTING_OPERAND_LIKE,

    // 14 +,  // todo tentative 14], x[
    EXPECTING_OPERATOR_LIKE,
} EXPECTING;

static bool check_and_update_expectations(const TokenType type, EXPECTING* expecting) {
    // todo: do
    // if expecting operand like do we have operand like next
    //  etc

    bool is_valid;
    switch (*expecting) {
        case EXPECTING_START:
        case EXPECTING_OPERAND_LIKE: {
            is_valid= is_valid_expr_start(type);
            break;
        }
        case EXPECTING_OPERATOR_LIKE: {
            is_valid= is_binary_operator(type) || is_postfix_operator(type) || type == TT_RPAREN;
            break;
        }
        default: assert(false);
    }

    if (!is_valid)
        return false;

    switch (*expecting) {
        case EXPECTING_START:
        case EXPECTING_OPERAND_LIKE: {
            if (is_prefix_operator(type) || type == TT_LPAREN) break;
            *expecting= EXPECTING_OPERATOR_LIKE; break;
        }
        case EXPECTING_OPERATOR_LIKE: {
            if (is_postfix_operator(type) || type == TT_RPAREN) break;
            *expecting= EXPECTING_OPERAND_LIKE; break;
        }
        default: assert(false);
    }

    return true;
}

typedef NodeRes (*ParseFunc)(void);
static ParseFunc parse_functions[]= {
    [TT_IDENTIFIER]= parse_identifier,
    [TT_KEYWORD_RET]= parse_ret,
    [TT_KEYWORD_BREAK]= parse_break,
    [TT_KEYWORD_CONTINUE]= parse_continue,
    [TT_KEYWORD_FOR]= parse_for,
    [TT_KEYWORD_WHILE]= parse_while,
    [TT_KEYWORD_DO]= parse_do,
    [TT_KEYWORD_IF]= parse_if,
    [TT_LBRACE]= parse_brace,
};

bool is_valid_expr_start(const TokenType type) {
    // TT_KEYWORD_DEFER: // todo: think about defer being an expression
    // TT_KEYWORD_IMPORT // todo: think about import being an expression
    if (is_prefix_operator(type)) return true;

    switch (type) {
        case TT_LBRACE:
        case TT_LPAREN:
        case TT_IDENTIFIER:
        case TT_KEYWORD_BREAK:
        case TT_KEYWORD_CONTINUE:
        case TT_KEYWORD_DATA:
        case TT_KEYWORD_DO:
        case TT_KEYWORD_FOR:
        case TT_KEYWORD_IF:
        case TT_KEYWORD_MARK:
        case TT_KEYWORD_RET:
        case TT_KEYWORD_UNMARK:
        case TT_KEYWORD_WHILE:
        case TT_KEYWORD_YIELD:
        case TT_TYPE_I1:
        case TT_TYPE_I2:
        case TT_TYPE_I4:
        case TT_TYPE_I8:
        case TT_TYPE_N1:
        case TT_TYPE_N2:
        case TT_TYPE_N4:
        case TT_TYPE_N8:
        case TT_TYPE_Q2:
        case TT_TYPE_Q4:
        case TT_TYPE_Q8:
        case TT_TYPE_Q16:
        case TT_TYPE_STRING:
        case TT_TYPE_BOOL:
        case TT_BOOLEAN_TRUE:
        case TT_BOOLEAN_FALSE:
        case TT_NULL:
        case TT_LIT_INT:
        case TT_LIT_FLOAT:
        case TT_LIT_STRING:
            return true;
        default:
            return false;
    }
}

// todo do the prefix and postfix and binary versions
#define PRECEDENCE_OFFSET (TT_INTERNAL_OPERATOR_START - 1)
static uint8_t PRECEDENCE[TT_INTERNAL_OPERATOR_END - TT_INTERNAL_OPERATOR_START + 1]= {
    [TT_OPERATOR_POSTFIX_PLUS_PLUS - PRECEDENCE_OFFSET]= 1,
    [TT_OPERATOR_POSTFIX_MINUS_MINUS - PRECEDENCE_OFFSET]= 1,
    [TT_OPERATOR_AMBIGUOUS_DOT - PRECEDENCE_OFFSET]= 1,

    [TT_OPERATOR_PREFIX_PLUS_PLUS - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_PREFIX_MINUS_MINUS - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_PREFIX_PLUS - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_PREFIX_SUBTRACT - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_BITWISE_NOT - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_PREFIX_EXCLAMATION - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_CAST - PRECEDENCE_OFFSET]= 2,
    [TT_OPERATOR_PREFIX_ADDRESS - PRECEDENCE_OFFSET]= 2,

    [TT_OPERATOR_POSTFIX_QUESTION - PRECEDENCE_OFFSET]= 3,
    [TT_OPERATOR_CARROT - PRECEDENCE_OFFSET]= 3,

    [TT_OPERATOR_MULT - PRECEDENCE_OFFSET]= 4,
    [TT_OPERATOR_DIV - PRECEDENCE_OFFSET]= 4,
    [TT_OPERATOR_MOD - PRECEDENCE_OFFSET]= 4,

    [TT_OPERATOR_BINARY_PLUS - PRECEDENCE_OFFSET]= 5,
    [TT_OPERATOR_BINARY_SUBTRACT - PRECEDENCE_OFFSET]= 5,

    [TT_OPERATOR_SHIFT_LEFT - PRECEDENCE_OFFSET]= 6,
    [TT_OPERATOR_BINARY_SHIFT_RIGHT - PRECEDENCE_OFFSET]= 6,

    [TT_OPERATOR_LESS - PRECEDENCE_OFFSET]= 7,
    [TT_OPERATOR_BINARY_MORE - PRECEDENCE_OFFSET]= 7,
    [TT_OPERATOR_LESS_EQ - PRECEDENCE_OFFSET]= 7,
    [TT_OPERATOR_MORE_EQ - PRECEDENCE_OFFSET]= 7,

    [TT_OPERATOR_EQ - PRECEDENCE_OFFSET]= 8,
    [TT_OPERATOR_NEQ - PRECEDENCE_OFFSET]= 8,

    [TT_OPERATOR_BITWISE_AND - PRECEDENCE_OFFSET]= 11,
    [TT_OPERATOR_BITWISE_XOR - PRECEDENCE_OFFSET]= 12,
    [TT_OPERATOR_BINARY_BITWISE_OR - PRECEDENCE_OFFSET]= 13,
    [TT_OPERATOR_LOGICAL_AND - PRECEDENCE_OFFSET]= 14,
    [TT_OPERATOR_LOGICAL_OR - PRECEDENCE_OFFSET]= 15,

    [TT_OPERATOR_TERNARY_CAPTURE - PRECEDENCE_OFFSET]= 16,

    [TT_OPERATOR_ASSIGN - PRECEDENCE_OFFSET]= 17,
    [TT_OPERATOR_ASSIGN_MULT - PRECEDENCE_OFFSET]= 17,
    [TT_OPERATOR_ASSIGN_DIV - PRECEDENCE_OFFSET]= 17,
    [TT_OPERATOR_ASSIGN_MOD - PRECEDENCE_OFFSET]= 17,
    [TT_OPERATOR_ASSIGN_PLUS - PRECEDENCE_OFFSET]= 17,
    [TT_OPERATOR_ASSIGN_SUBTRACT - PRECEDENCE_OFFSET]= 17,
};

static uint8_t get_prec(TokenType type) {
    return PRECEDENCE[type - PRECEDENCE_OFFSET];
}

static bool is_prefix_operator(TokenType type) {
    return type > TT_INTERNAL_OPERATOR_PREFIX_START &&
           type < TT_INTERNAL_OPERATOR_PREFIX_END;
}

static bool is_postfix_operator(TokenType type) {
    return type > TT_INTERNAL_OPERATOR_POSTFIX_START &&
           type < TT_INTERNAL_OPERATOR_POSTFIX_END;
}

static bool is_unary_operator(TokenType type) {
    return type > TT_INTERNAL_OPERATOR_UNARY_START &&
           type < TT_INTERNAL_OPERATOR_UNARY_END;
}

static bool is_assign_operator(const TokenType type) {
    return type > TT_INTERNAL_OPERATOR_ASSIGN_START &&
           type < TT_INTERNAL_OPERATOR_ASSIGN_END;
}

static bool is_right_assoc(const TokenType type) {
    return is_prefix_operator(type) ||
           is_assign_operator(type);
}

static bool is_binary_operator(const TokenType type) {
    return type > TT_INTERNAL_OPERATOR_BINARY_START &&
           type < TT_INTERNAL_OPERATOR_BINARY_END;
}

static bool is_operator(const TokenType type) {
    return type > TT_INTERNAL_OPERATOR_START &&
           type < TT_INTERNAL_OPERATOR_END;
}

static NodeRes build_unary_operator(const Token operator, NodeIndexArray* output_s) {
    if (output_s->pos < 1) {
        parse_error("Tried to form a unary operator, but there are no operands on the stack.\nOperator: ");
        PRINT_TOKEN(operator);
        newline();
        return NODE_FAIL(EC_FAIL);
    }

    const NodeIndex ni= node_create(NT_EXPR_OPERATOR_UNARY, operator);
    SET_LHS(NodeIndex_array_pop(output_s), ni);

    return NODE_SUCC(ni);
}

static NodeRes build_binary_operator(const Token operator, NodeIndexArray* output_s) {
    if (output_s->pos < 2) {
        printf("-----\n");
        print_node(NodeIndex_array_get(output_s, 0));
        fflush(stdout);
        parse_error("Tried to form a binary operator, but there are only one or fewer operands on the stack.\nOperator: ");
        PRINT_TOKEN(operator);
        newline();
        return NODE_FAIL(EC_FAIL);
    }

    const NodeIndex ni= node_create(NT_EXPR_OPERATOR_BINARY, operator);
    SET_LHS(NodeIndex_array_pop(output_s), ni);
    SET_RHS(NodeIndex_array_pop(output_s), ni);

    return NODE_SUCC(ni);
}

static NodeRes build_operator(const Token operator, NodeIndexArray* output_s) {
    // todo: support non binary operators
    if (is_binary_operator(operator.type)) return build_binary_operator(operator, output_s);
    if (is_unary_operator(operator.type)) return build_unary_operator(operator, output_s);

    assert(false);
}

static void disambiguate_operator_type(Token* token, TokenArray* operator_s, EXPECTING expecting) {
    // -
    // a-b
    // -x
    // x++
    // a?b
    switch (token->type) {
        case TT_OPERATOR_AMBIGUOUS_PLUS: {
            token->type= (expecting == EXPECTING_OPERATOR_LIKE) ? TT_OPERATOR_BINARY_PLUS : TT_OPERATOR_PREFIX_PLUS;
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_SUBTRACT: {
            token->type= (expecting == EXPECTING_OPERATOR_LIKE) ? TT_OPERATOR_BINARY_SUBTRACT : TT_OPERATOR_PREFIX_SUBTRACT;
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_MORE: {
            token->type= (expecting == EXPECTING_OPERATOR_LIKE) ? TT_OPERATOR_BINARY_MORE : TT_OPERATOR_PREFIX_ADDRESS;
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_DOT: {
            const Token n= peek();
            if (n.type == TT_IDENTIFIER ||
                n.type == TT_LIT_INT
            ) {
                token->type= TT_OPERATOR_BINARY_DOT;
            } else {
                token->type= TT_OPERATOR_POSTFIX_DOT;
            }
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_QUESTION: {
            const Token n= peek();
            if (is_valid_expr_start(n.type)) {
                token->type= TT_OPERATOR_BINARY_QUESTION;
            } else {
                token->type= TT_OPERATOR_POSTFIX_QUESTION;
            }
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_EXCLAMATION: {
            if (expecting == EXPECTING_OPERAND_LIKE) {
                token->type= TT_OPERATOR_PREFIX_EXCLAMATION;
                break;
            }

            const Token n= peek();
            if (is_valid_expr_start(n.type)) {
                token->type= TT_OPERATOR_BINARY_EXCLAMATION;
            } else {
                token->type= TT_OPERATOR_POSTFIX_EXCLAMATION;
            }
            break;
        }

        case TT_OPERATOR_AMBIGUOUS_SHIFT_RIGHT: {
            if (expecting == EXPECTING_OPERATOR_LIKE) {
                token->type= TT_OPERATOR_BINARY_SHIFT_RIGHT;
                break;
            }

            const Token first_token= (Token) {.type= TT_OPERATOR_PREFIX_ADDRESS, .buffer_index= token->buffer_index};
            Token_array_add(operator_s, first_token);

            token->type= TT_OPERATOR_PREFIX_ADDRESS;
            token->buffer_index++;
            break;
        }

        case TT_OPERATOR_AMBIGUOUS_BITWISE_OR: // todo x|y|z
            break;

        case TT_OPERATOR_AMBIGUOUS_PLUS_PLUS: {
            token->type= (expecting == EXPECTING_OPERATOR_LIKE) ? TT_OPERATOR_POSTFIX_PLUS_PLUS : TT_OPERATOR_PREFIX_PLUS_PLUS;
            break;
        }
        case TT_OPERATOR_AMBIGUOUS_MINUS_MINUS: {
            token->type= (expecting == EXPECTING_OPERATOR_LIKE) ? TT_OPERATOR_POSTFIX_MINUS_MINUS : TT_OPERATOR_PREFIX_MINUS_MINUS;
            break;
        }
        default: break;
    }
}

NodeRes shunt(bool allow_trailing_parens) {
    Token c;

    NodeIndexArray output_s= NodeIndex_array_construct(5);
    TokenArray operator_s= Token_array_construct(5);
    EXPECTING expecting= EXPECTING_START;
    while (c= current(), c.type != TT_DELIMITER && c.type != TT_EOTS) {
        // literal numbers
        // function calls -- identifier()
        // variable names -- identifiers
        // braces -- {}
        // control flow keywords -- ret, continue, break
        // if statements
        // whiles, do whiles, for loops
        // binary operators
        // unary operators

        disambiguate_operator_type(&c, &operator_s, expecting);
        print_token(c, -1, -1, &tokens);
        newline();
        
        // todo
        // types -- value is their size
        // maybe defer
        // maybe import
        if (!check_and_update_expectations(c.type, &expecting)) {
            break;
            return NODE_FAIL(EC_FAIL);
        }

        // todo: if types are valid in expressions i.e. they give their size
        //  then how can ? be used on a type and in an expression
        //  it is ambiguous
        // y: Type      y: i4?
        // x:= y?       x:= y?
        if (is_valid_type_start(c.type) && c.type != TT_IDENTIFIER) {
            const NodeRes res= parse_type();
            PROP_FAIL(res)

            NodeIndex_array_add(&output_s, res.index);
            continue;
        }

        if (is_operator(c.type)) {
            consume();
            const Token o1= c;

            while (!Token_array_empty(&operator_s)) {
                Token o2 = Token_array_get(&operator_s, operator_s.pos - 1);
                if (o2.type == TT_LPAREN) break;

                const uint8_t o1_pres= get_prec(o1.type);
                const uint8_t o2_pres= get_prec(o2.type);

                if (o2_pres > o1_pres) break;
                if (o2_pres == o1_pres && is_right_assoc(o1.type)) break;

                Token o2_mirror= Token_array_pop(&operator_s);
                assert(memcmp(&o2, &o2_mirror, sizeof(Token)) == 0);

                const NodeRes op_res= build_operator(o2, &output_s);
                PROP_FAIL(op_res);

                NodeIndex_array_add(&output_s, op_res.index);
            }

            Token_array_add(&operator_s, o1);
            continue;
        }

        bool end= false;
        switch (c.type) {
            case TT_LIT_FLOAT:
            case TT_LIT_INT: {
                const Token tok= consume();
                // todo: maybe split the ints and floats?
                const NodeIndex ni= node_create(NT_EXPR_LITERAL, tok);
                NodeIndex_array_add(&output_s, ni);

                break;
            }

            case TT_IDENTIFIER: {
                // variable
                // function call
                // an array index x[]
                const NodeRes ident_res= parse_identifier();
                PROP_FAIL(ident_res);

                NodeIndex_array_add(&output_s, ident_res.index);
                break;
            }
            case TT_KEYWORD_RET:
            case TT_KEYWORD_BREAK:
            case TT_KEYWORD_CONTINUE:
            case TT_KEYWORD_FOR:
            case TT_KEYWORD_WHILE:
            case TT_KEYWORD_DO:
            case TT_KEYWORD_IF:
            case TT_LBRACE: {
                const ParseFunc func= parse_functions[c.type];
                assert(func);

                const NodeRes res= func();
                PROP_FAIL(res)

                NodeIndex_array_add(&output_s, res.index);
                break;
            }

            case TT_LPAREN: {
                Token_array_add(&operator_s, consume());
                break;
            }
            case TT_RPAREN: {
                const Token r_paren_tok= consume();

                if (Token_array_empty(&operator_s)) {
                    if (!allow_trailing_parens) {
                        parse_error("Found mismatching parenthesis in expression.\nExtra right paren: ");
                        PRINT_TOKEN(r_paren_tok);
                        newline();
                        return NODE_FAIL(EC_FAIL);
                    }
                    break;
                }

                Token t;
                while (!Token_array_empty(&operator_s)) {
                    t= Token_array_get(&operator_s, operator_s.pos - 1);
                    if (t.type == TT_LPAREN) break;

                    const NodeRes op_res= build_operator(Token_array_pop(&operator_s), &output_s);
                    PROP_FAIL(op_res)

                    NodeIndex_array_add(&output_s, op_res.index);
                }

                if (t.type != TT_LPAREN) {
                    if (!allow_trailing_parens) {
                        parse_error("Found mismatching parenthesis in expression.\nExtra right paren: ");
                        PRINT_TOKEN(r_paren_tok);
                        newline();
                        return NODE_FAIL(EC_FAIL);
                    }
                    break;
                }

                Token_array_pop(&operator_s);
                break;
            }

            default: {
                end= true;
                break;
            }
        }

        if (end) break;
    }

    while (!Token_array_empty(&operator_s)) {
        const Token op= Token_array_pop(&operator_s);
        if (op.type == TT_LPAREN) {
            parse_error("Mismatched parenthesis, got an lparen without rparen to match.\nLParen: ");
            PRINT_TOKEN(op);
            newline();
            return NODE_FAIL(EC_FAIL);
        }

        const NodeRes op_res= build_operator(op, &output_s);
        PROP_FAIL(op_res)

        NodeIndex_array_add(&output_s, op_res.index);
    }

    return NODE_SUCC(NodeIndex_array_pop(&output_s));
}