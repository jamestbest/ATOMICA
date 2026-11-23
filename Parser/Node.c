//
// Created by jamescoward on 30/10/2026.
//

#include "Node.h"

#include <pthread.h>
#include <stdio.h>

#include "SharedParsing.h"
#include "Logger/Logger.h"

typedef enum {
    NPV_LAST_CHILD, // `-
    NPV_CHILD, // |-
    NPV_PASS, // |
    NPV_NONE, //
} NodePrintingValue;

static __thread uint64_t npv_buffer[10];
static __thread uint32_t npv_buffer_pos;

static void print_token_as_node(Token tok);
static void print_node(NodeIndex ni);

static NodePrintingValue npv_buffer_get(size_t idx) {
    const uint32_t x= idx / 32;
    const uint32_t y= idx % 32;

    return (npv_buffer[x] >> (y << 1)) & 0b11;
}

static void npv_buffer_set(const size_t idx, const NodePrintingValue value) {
    if (idx > (sizeof(npv_buffer) * 4)) assert(false && "Nodes are too deeply nested");

    const uint32_t x= idx / 32;
    const uint32_t y= idx % 32;

    npv_buffer[x] &= ~(0b11 << (y << 1));
    npv_buffer[x] |= ((value & 0b11) << (y << 1));
}

static void npv_buffer_add(const NodePrintingValue value) {
    npv_buffer_set(npv_buffer_pos++, value);
}

static void npv_buffer_pop() {
    npv_buffer_pos--;
}

static void print_node_prefix() {
    for (int i = 0; i < npv_buffer_pos; ++i) {
        const NodePrintingValue value= npv_buffer_get(i);

        switch (value) {
            case NPV_LAST_CHILD: logger_fadd("`-"); break;
            case NPV_CHILD: logger_fadd("|-"); break;
            case NPV_PASS: logger_fadd("| "); break;
            case NPV_NONE: logger_fadd("  "); break;
            default:
                assert(false);
        }
    }
}

const char* node_type_string(const NodeType type) {
    switch (type) {
        case NT_ROOT: return "ROOT";
        case NT_COMPOUND_STATEMENT: return "COMPOUND_STATEMENT";
        case NT_VAR_DECL: return "VAR_DECL";
        case NT_CONST_DECL: return "CONST_DECL";
        case NT_LAMBDA: return "LAMBDA";
        case NT_LAMBDA_HEADER: return "LAMBDA_HEADER";
        case NT_PARAM: return "PARAM";
        case NT_TYPED_IDENTIFIER: return "TYPED_IDENTIFIER";
        case NT_TYPE_ERRORABLE: return "TYPE_ERRORABLE";
        case NT_TYPE_ARRAY_STATIC: return "TYPE_ARRAY_STATIC";
        case NT_TYPE_ARRAY_DYNAMIC: return "TYPE_ARRAY_DYNAMIC";
        case NT_TYPE_ARRAY_LINK: return "TYPE_ARRAY_LINK";
        case NT_TYPE_ARRAY_POOL: return "TYPE_ARRAY_POOL";
        case NT_TYPE_ARRAY_SLICE: return "TYPE_ARRAY_SLICE";
        case NT_TYPE_NULLABLE: return "TYPE_NULLABLE";
        case NT_TYPE_POINTER: return "TYPE_POINTER";
        case NT_TYPE_BASE: return "TYPE_BASE";
        case NT_TYPE_LINK: return "TYPE_LINK";
        case NT_TYPE_CONST: return "TYPE_CONST";
        case NT_TYPE_DATA: return "TYPE_DATA";
        case NT_DATA_FIELDS: return "DATA_FIELDS";
        case NT_DATA_PARAMETERISED_FIELDS: return "DATA_PARAMETERISED_FIELDS";
        case NT_DATA_FIELD: return "DATA_FIELD";
        case NT_DATA_VARIANT: return "DATA_VARIANT";
        case NT_TYPE_MARKED: return "TYPE_MARKED";
        case NT_EXPR_LITERAL: return "EXPR_LITERAL";
        case NT_EXPR_LITERAL_ARRAY: return "EXPR_LITERAL_ARRAY";
        case NT_EXPR_OPERATOR_BINARY: return "EXPR_OPERATOR_BINARY";
        case NT_EXPR_OPERATOR_UNARY: return "EXPR_OPERATOR_UNARY";
        case NT_EXPR_OPERATOR_TERNARY: return "NT_EXPR_OPERATOR_TERNARY";
        case NT_EXPR_VARIABLE: return "EXPR_VARIABLE";
        case NT_EXPR_FUNC_CALL: return "EXPR_FUNC_CALL";
        case NT_EXPR_ARRAY_INDEX: return "EXPR_ARRAY_INDEX";
        case NT_EXPR_RETURN: return "EXPR_RETURN";
        case NT_EXPR_BREAK: return "EXPR_BREAK";
        case NT_EXPR_CONTINUE: return "EXPR_CONTINUE";
        case NT_IMPORT_STATEMENT: return "IMPORT_STATEMENT";
        case NT_IF_EXPR: return "IF_EXPR";
        case NT_ELSE_EXPR: return "ELSE_EXPR";
        case NT_DO_WHILE_EXPR: return "DO_WHILE_EXPR";
        case NT_WHILE_EXPR: return "WHILE_EXPR";
        case NT_FOR_EACH_EXPR: return "FOR_EACH_EXPR";
        case NT_FOR_EXPR: return "FOR_EXPR";
        case NT_EXPR_RETURN_ERROR: return "EXPR_RETURN_ERROR";
        case NT_EXPR_YIELD: return "EXPR_YIELD";
        case NT_EXPR_YIELD_ERROR: return "EXPR_YIELD_ERROR";
        case NT_EXPR_BREAK_ERROR: return "EXPR_BREAK_ERROR";
        case NT_EXPR_DEFER: return "EXPR_DEFER";
        case NT_EXPR_DEFER_ERROR: return "EXPR_DEFER_ERROR";
        case NT_IDENTIFIER: return "IDENTIFIER";
        case NT_IDENTIFIER_LIST: return "IDENTIFIER_LIST";
        case NT_EXPR_CODE_LITERAL: return "CODE_LITERAL";
        case NT_IDENTIFIER_LITERAL: return "IDENTIFIER_LITERAL";
        default:
            assert(false && "You didn't add the NT to this function. BAD.\n");
    }
}

#define UNWRAP_LR(left, right) \
    const NodeIndex left= GET_LHS(ni); \
    const NodeIndex right= GET_RHS(ni);

#define UNWRAP_L(left) \
    const NodeIndex left= GET_LHS(ni); \

NodeIf get_node_if(const NodeIndex ni) {
    UNWRAP_LR(start, end)

    const NodeIndex cond= NodeIndex_array_get(&extra_array, start);
    const NodeIndex output= NodeIndex_array_get(&extra_array, start + 1);
    const NodeIndex else_= NodeIndex_array_get(&extra_array, start + 2);

    return (NodeIf) {
        .cond= cond,
        .output= output,
        .else_= else_
    };
}

NodeFuncCall get_node_func_call(const NodeIndex ni) {
    UNWRAP_LR(start, end)

    NodeFuncCall node= (NodeFuncCall) {
        .arg_count= end - start + 1,
        .arg_start= start
    };

    return node;
}

static void print_child(const NodeIndex ni) {
    npv_buffer_add(NPV_CHILD);
    print_node(ni);
    npv_buffer_pop();
}

static void print_child_token(const Token tok) {
    npv_buffer_add(NPV_CHILD);
    print_token_as_node(tok);
    npv_buffer_pop();
}

static void print_last_child(const NodeIndex ni) {
    npv_buffer_add(NPV_LAST_CHILD);
    print_node(ni);
    npv_buffer_pop();
}

static void print_last_child_token(const Token tok) {
    npv_buffer_add(NPV_LAST_CHILD);
    print_token_as_node(tok);
    npv_buffer_pop();
}

static void print_child_ambiguous(const NodeIndex ni, bool is_last) {
    if (is_last) print_last_child(ni);
    else print_child(ni);
}

static void print_child_token_ambiguous(const Token tok, bool is_last) {
    if (is_last) print_last_child_token(tok);
    else print_child_token(tok);
}

#define GET_EXTRA_START_AND_END  UNWRAP_LR(extra_start, extra_end)

static void print_token_as_node(const Token tok) {
    print_node_prefix();
    print_token(tok, -1, -1, source);

    logger_fadd("\n");
}

void print_root_node(const NodeIndex ni) {
    print_node(ni);
    logger_commit();
}

void print_node(const NodeIndex ni) {
    print_node_prefix();

    if (npv_buffer_pos != 0) {
        if (npv_buffer_get(npv_buffer_pos - 1) == NPV_LAST_CHILD) {
            npv_buffer_set(npv_buffer_pos - 1, NPV_NONE);
        } else {
            npv_buffer_set(npv_buffer_pos - 1, NPV_PASS);
        }
    }

    const NodeType type= GET_TYPE(ni);

    logger_fadd("<%s:", node_type_string(type));
    const Token tok= GET_TOKEN(ni);
    print_token(tok, -1, -1, source);
    logger_fadd(" ");

    switch (type) {
        case NT_ROOT:
        case NT_COMPOUND_STATEMENT: {
            GET_EXTRA_START_AND_END

            if (extra_start == NODE_INDEX_NONE ||
                extra_end == NODE_INDEX_NONE
            ) {
                logger_fadd("No children>\n");
                break;
            }

            logger_fadd(">\n");
            for (size_t i = extra_start; i < extra_end + 1; ++i) {
                const NodeIndex c_ni= NodeIndex_array_get(&extra_array, i);
                print_child_ambiguous(c_ni, i == extra_end);
            }
            break;
        }
        case NT_VAR_DECL:
        case NT_CONST_DECL: {
            logger_fadd("Ident= ");
            print_token_value(tok);

            const NodeIndex lhs= GET_LHS(ni);
            if (lhs == NODE_INDEX_NONE) {
                logger_fadd(" (inferred type)");
            }

            const NodeIndex rhs= GET_RHS(ni);
            if (rhs == NODE_INDEX_NONE) {
                logger_fadd(" (no value)");
            }

            logger_fadd(">\n");

            if (lhs != NODE_INDEX_NONE) {
                print_child_ambiguous(GET_LHS(ni), rhs == NODE_INDEX_NONE);
            }

            if (rhs != NODE_INDEX_NONE) {
                print_last_child(GET_RHS(ni));
            }

            break;
        }
        case NT_LAMBDA: {
            logger_fadd(">\n");

            UNWRAP_LR(header, body)

            print_child(header);
            print_last_child(body);
            break;
        }
        case NT_LAMBDA_HEADER: {
            logger_fadd(">\n");

            UNWRAP_LR(start, end)

            const NodeIndex return_type= end;

            if (start != return_type) {
                // there are params
                for (NodeIndex i = start; i < end; ++i) {
                    const NodeIndex param= NodeIndex_array_get(&extra_array, i);
                    print_child(param);
                }
            }

            print_last_child(NodeIndex_array_get(&extra_array, return_type));

            break;
        }
        case NT_PARAM: {
            UNWRAP_LR(return_type, default_value)

            logger_fadd(">\n");

            print_child_ambiguous(return_type, default_value == NODE_INDEX_NONE);

            if (default_value != NODE_INDEX_NONE) {
                print_last_child(default_value);
            }

            break;
        }
        case NT_TYPED_IDENTIFIER: {
            logger_fadd("Ident= ");
            print_token_value(tok);

            logger_fadd(">\n");

            print_last_child(GET_LHS(ni));
            break;
        }
        case NT_TYPE_ERRORABLE: {
            logger_fadd(">\n");

            UNWRAP_LR(error_as_index, return_type)
            const Token error_token= NODE_INDEX_AS_TOKEN(error_as_index);

            if (error_as_index != NODE_INDEX_NONE) {
                print_child_token(error_token);
            }

            print_last_child(return_type);
            break;
        }
        case NT_TYPE_ARRAY_STATIC: {
            logger_fadd(">\n");

            UNWRAP_LR(inner_type, size)

            print_child(inner_type);
            print_last_child(size);

            break;
        }
        case NT_TYPE_ARRAY_DYNAMIC:
        case NT_TYPE_ARRAY_LINK:
        case NT_TYPE_ARRAY_POOL:
        case NT_TYPE_ARRAY_SLICE: {
            logger_fadd(">\n");
            UNWRAP_L(inner_type)

            print_last_child(inner_type);

            break;
        }
        case NT_TYPE_NULLABLE:
        case NT_TYPE_POINTER:
        case NT_TYPE_MARKED: {
            logger_fadd(">\n");

            UNWRAP_L(inner_type)

            print_last_child(inner_type);

            break;
        }
        case NT_TYPE_BASE:
        case NT_TYPE_LINK: {
            logger_fadd(">\n");
            break;
        }
        case NT_TYPE_CONST: {
            logger_fadd(">\n");

            UNWRAP_L(inner_type)

            if (inner_type != NODE_INDEX_NONE) {
                const Token t= NODE_INDEX_AS_TOKEN(inner_type);
                print_last_child_token(t);
            }

            break;
        }
        case NT_TYPE_DATA: {
            logger_fadd(">\n");

            UNWRAP_LR(param_fields, fields);

            if (param_fields != NODE_INDEX_NONE) {
                print_child(param_fields);
            }

            print_last_child(fields);

            break;
        }
        case NT_DATA_FIELDS:
        case NT_DATA_PARAMETERISED_FIELDS: {
            UNWRAP_LR(start, end);

            logger_fadd(">\n");

            if (start == NODE_INDEX_NONE ||
                end == NODE_INDEX_NONE
            ) break;

            for (size_t i = start; i < end + 1; ++i) {
                const NodeIndex c_ni= NodeIndex_array_get(&extra_array, i);

                print_child_ambiguous(c_ni, i == end);
            }

            break;
        }
        case NT_DATA_FIELD: {
            logger_fadd("Ident= ");
            print_token_value(tok);

            logger_fadd(">\n");

            UNWRAP_LR(type, default_value)

            print_child_ambiguous(type, default_value == NODE_INDEX_NONE);

            if (default_value != NODE_INDEX_NONE) {
                print_last_child(default_value);
            }

            break;
        }
        case NT_DATA_VARIANT: {
            logger_fadd("Ident= ");
            print_token_value(tok);

            logger_fadd(">\n");

            UNWRAP_LR(start, end);
            const NodeIndex default_value= end;

            const bool has_default= default_value != NODE_INDEX_NONE;

            for (size_t i= start; i < end; ++i) {
                print_child_ambiguous(
                    NodeIndex_array_get(&extra_array, i),
                    !has_default && i == end - 1
                );
            }

            if (default_value != NODE_INDEX_NONE) {
                print_last_child(NodeIndex_array_get(&extra_array, default_value));
            }

            break;
        }
        case NT_EXPR_LITERAL: {
            logger_fadd(">\n");
            break;
        }
        case NT_EXPR_LITERAL_ARRAY: {
            logger_fadd(">\n");

            UNWRAP_LR(start, end)

            for (size_t i = start; i < end + 1; ++i) {
                const NodeIndex elem= NodeIndex_array_get(&extra_array, i);

                print_child_ambiguous(
                    elem,
                    i == end
                );
            }

            break;
        }
        case NT_EXPR_OPERATOR_BINARY: {
            logger_fadd(">\n");

            UNWRAP_LR(left, right)

            print_child(left);
            print_last_child(right);

            break;
        }
        case NT_EXPR_OPERATOR_UNARY: {
            logger_fadd(">\n");

            UNWRAP_L(left);
            print_last_child(left);

            break;
        }
        case NT_EXPR_OPERATOR_TERNARY: {
            logger_fadd(">\n");

            UNWRAP_L(start)
            print_child(NodeIndex_array_get(&extra_array, start));
            print_child(NodeIndex_array_get(&extra_array, start + 1));
            print_last_child(NodeIndex_array_get(&extra_array, start + 2));

            break;
        }
        case NT_EXPR_VARIABLE: {
            logger_fadd(">\n");
            break;
        }
        case NT_EXPR_FUNC_CALL: {
            const NodeFuncCall node= get_node_func_call(ni);

            logger_fadd("args: %zu ", node.arg_count);
            logger_fadd(">\n");

            for (size_t i = 0; i < node.arg_count; ++i) {
                const size_t arg_idx= node.arg_start + i;
                const NodeIndex arg= NodeIndex_array_get(&extra_array, arg_idx);

                print_child_ambiguous(arg, i == node.arg_count - 1);
            }

            break;
        }
        case NT_EXPR_ARRAY_INDEX: {
            logger_fadd(">\n");

            UNWRAP_L(expr)

            print_last_child(expr);

            break;
        }
        case NT_EXPR_RETURN:
        case NT_EXPR_BREAK:
        case NT_EXPR_RETURN_ERROR:
        case NT_EXPR_BREAK_ERROR:
        case NT_EXPR_YIELD:
        case NT_EXPR_YIELD_ERROR: {
            logger_fadd(">\n");

            UNWRAP_L(expr)

            if (expr != NODE_INDEX_NONE) {
                print_last_child(expr);
            }

            break;
        }
        case NT_EXPR_CONTINUE: {
            logger_fadd(">\n");
            break;
        }
        case NT_IMPORT_STATEMENT: {
            logger_fadd(">\n");

            UNWRAP_L(string_as_index)
            const Token string_token= NODE_INDEX_AS_TOKEN(string_as_index);

            print_last_child_token(string_token);

            break;
        }
        case NT_IF_EXPR: {
            logger_fadd(">\n");

            const NodeIf node= get_node_if(ni);

            print_child(node.cond);
            print_child_ambiguous(node.output, node.else_ == NODE_INDEX_NONE);
            if (node.else_ != NODE_INDEX_NONE) {
                print_last_child(node.else_);
            }

            break;
        }
        case NT_ELSE_EXPR: {
            logger_fadd(">\n");

            UNWRAP_L(output)

            print_last_child(output);

            break;
        }
        case NT_DO_WHILE_EXPR:
        case NT_WHILE_EXPR: {
            logger_fadd(">\n");

            UNWRAP_LR(cond, output)

            print_child(cond);
            print_last_child(output);

            break;
        }
        case NT_FOR_EACH_EXPR: {
            logger_fadd(">\n");

            UNWRAP_LR(start, end);

            const NodeIndex elem= NodeIndex_array_get(&extra_array, start);
            const NodeIndex array= NodeIndex_array_get(&extra_array, start + 1);
            const NodeIndex with_name= NodeIndex_array_get(&extra_array, start + 2);
            const NodeIndex output_expr= NodeIndex_array_get(&extra_array, start + 3);

            if (array == NODE_INDEX_NONE) {
                print_child(elem);
            } else {
                print_child_token(NODE_INDEX_AS_TOKEN(elem));
            }

            if (array != NODE_INDEX_NONE) {
                print_child_token(NODE_INDEX_AS_TOKEN(array));
            }

            if (with_name != NODE_INDEX_NONE) {
                print_child_token(NODE_INDEX_AS_TOKEN(with_name));
            }

            print_last_child(output_expr);

            break;
        }
        case NT_FOR_EXPR: {
            logger_fadd(">\n");

            UNWRAP_L(start);

            const NodeIndex setup_expr= NodeIndex_array_get(&extra_array, start);
            const NodeIndex condition_expr= NodeIndex_array_get(&extra_array, start + 1);
            const NodeIndex iteration_expr= NodeIndex_array_get(&extra_array, start + 2);

            if (setup_expr != NODE_INDEX_NONE) {
                print_child_ambiguous(
                    setup_expr,
                    condition_expr == NODE_INDEX_NONE && iteration_expr == NODE_INDEX_NONE
                );
            }

            if (condition_expr != NODE_INDEX_NONE) {
                print_child_ambiguous(
                    condition_expr,
                    iteration_expr == NODE_INDEX_NONE
                );
            }

            if (iteration_expr != NODE_INDEX_NONE) {
                print_last_child(iteration_expr);
            }

            break;
        }
        case NT_EXPR_DEFER:
        case NT_EXPR_DEFER_ERROR:
            logger_fadd(">\n");

            UNWRAP_L(statement)

            print_last_child(statement);
            break;
        case NT_IDENTIFIER: {
            logger_fadd(">\n");

            break;
        }
        case NT_IDENTIFIER_LIST: {
            logger_fadd(">\n");

            UNWRAP_LR(start, end);

            for (size_t i = start; i < end + 1; ++i) {
                const NodeIndex child= NodeIndex_array_get(&extra_array, i);
                print_child_ambiguous(child, i == end);
            }

            break;
        }
        case NT_EXPR_CODE_LITERAL: {
            UNWRAP_LR(start, end)
            const MyTokenSource token_source= *(MyTokenSource*)NodeIndex_array_ptr(&extra_array, start);
            const size_t token_count= token_source.end_idx - token_source.start_idx + 1;
            logger_fadd("(%zu token%s) ", token_count, token_count > 1 ? "s": "");
            logger_fadd(">\n");

            break;
        }
        case NT_IDENTIFIER_LITERAL: {
            logger_fadd(">\n");
            break;
        }
        default:
            assert(false);
    }
}
