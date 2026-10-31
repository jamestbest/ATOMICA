//
// Created by jamescoward on 30/10/2026.
//

#include "Node.h"

#include <stdio.h>

#include "SharedParsing.h"

typedef enum {
    NPV_LAST_CHILD, // `-
    NPV_CHILD, // |-
    NPV_PASS, // |
    NPV_NONE, //
} NodePrintingValue;

static uint64_t npv_buffer[10];
static uint32_t npv_buffer_pos;

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
            case NPV_LAST_CHILD: printf("`-"); break;
            case NPV_CHILD: printf("|-"); break;
            case NPV_PASS: printf("| "); break;
            case NPV_NONE: printf("  "); break;
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
        case NT_EXPR_OPERATOR_BINARY: return "EXPR_OPERATOR_BINARY";
        case NT_EXPR_OPERATOR_UNARY: return "EXPR_OPERATOR_UNARY";
        case NT_EXPR_VARIABLE: return "EXPR_VARIABLE";
        case NT_EXPR_FUNC_CALL: return "EXPR_FUNC_CALL";
        case NT_EXPR_ARRAY_INDEX: return "EXPR_ARRAY_INDEX";
        case NT_EXPR_RETURN: return "EXPR_RETURN";
        case NT_EXPR_BREAK: return "EXPR_BREAK";
        case NT_EXPR_CONTINUE: return "EXPR_CONTINUE";
        default:
            assert(false && "You didn't add the NT to this function. BAD.\n");
    }
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

    printf("<%s:", node_type_string(type));
    const Token tok= GET_TOKEN(ni);
    print_token(tok, -1, -1, &tokens);

    switch (type) {
        case NT_ROOT:
        case NT_COMPOUND_STATEMENT: {
            const size_t extra_start= GET_LHS(ni);
            const size_t extra_end= GET_RHS(ni);

            if (extra_start == NODE_INDEX_NONE ||
                extra_end == NODE_INDEX_NONE) {
                printf("No children>\n");
                break;
            }

            printf(">\n");
            for (size_t i = extra_start; i < extra_end + 1; ++i) {
                const NodeIndex c_ni= NodeIndex_array_get(&extra_array, i);
                npv_buffer_add(i == extra_end ? NPV_LAST_CHILD : NPV_CHILD);
                print_node(c_ni);
                npv_buffer_pop();
            }
            break;
        }
        case NT_VAR_DECL:
        case NT_CONST_DECL: {
            printf(" Ident= ");
            print_token_value(tok);

            const NodeIndex lhs= GET_LHS(ni);
            if (lhs == NODE_INDEX_NONE) {
                printf(" (inferred type)");
            }

            const NodeIndex rhs= GET_RHS(ni);
            if (rhs == NODE_INDEX_NONE) {
                printf(" (no value)");
            }

            printf(">\n");

            if (lhs != NODE_INDEX_NONE) {
                npv_buffer_add(NPV_CHILD);
                print_node(GET_LHS(ni));
                npv_buffer_pop();
            }

            if (rhs != NODE_INDEX_NONE) {
                npv_buffer_add(NPV_LAST_CHILD);
                print_node(GET_RHS(ni));
                npv_buffer_pop();
            }

            break;
        }
        case NT_LAMBDA:
            break;
        case NT_LAMBDA_HEADER:
            break;
        case NT_PARAM:
            break;
        case NT_TYPED_IDENTIFIER: {
            printf(" Ident= ");
            print_token_value(tok);

            printf(">\n");
            npv_buffer_add(NPV_LAST_CHILD);
            print_node(GET_LHS(ni));
            npv_buffer_pop();
            break;
        }
        case NT_TYPE_ERRORABLE:
            break;
        case NT_TYPE_ARRAY_STATIC:
            break;
        case NT_TYPE_ARRAY_DYNAMIC:
            break;
        case NT_TYPE_ARRAY_LINK:
            break;
        case NT_TYPE_ARRAY_POOL:
            break;
        case NT_TYPE_ARRAY_SLICE:
            break;
        case NT_TYPE_NULLABLE:
            break;
        case NT_TYPE_POINTER:
            break;
        case NT_TYPE_BASE:
            break;
        case NT_TYPE_LINK: {
            printf(">\n");
        }
            break;
        case NT_TYPE_CONST:
            break;
        case NT_TYPE_DATA: {
            const NodeIndex lhs= GET_LHS(ni);
            const NodeIndex rhs= GET_RHS(ni);

            printf(">\n");

            if (lhs != NODE_INDEX_NONE) {
                npv_buffer_add(NPV_CHILD);
                print_node(lhs);
                npv_buffer_pop();
            }

            npv_buffer_add(NPV_LAST_CHILD);
            print_node(rhs);
            npv_buffer_pop();

            break;
        }
        case NT_DATA_FIELDS:
        case NT_DATA_PARAMETERISED_FIELDS: {
            const NodeIndex extra_start= GET_LHS(ni);
            const NodeIndex extra_end= GET_RHS(ni);

            printf(">\n");

            if (extra_start == NODE_INDEX_NONE ||
                extra_end == NODE_INDEX_NONE
            ) break;

            for (size_t i = extra_start; i < extra_end + 1; ++i) {
                const NodeIndex c_ni= NodeIndex_array_get(&extra_array, i);

                npv_buffer_add(i == extra_end ? NPV_LAST_CHILD : NPV_CHILD);
                print_node(c_ni);
                npv_buffer_pop();
            }

            break;
        }
        case NT_DATA_FIELD: {
            printf(" Ident= ");
            print_token_value(tok);

            printf(">\n");

            const NodeIndex default_value= GET_RHS(ni);

            npv_buffer_add(default_value == NODE_INDEX_NONE ? NPV_LAST_CHILD : NPV_CHILD);
            print_node(GET_LHS(ni));
            npv_buffer_pop();

            if (default_value != NODE_INDEX_NONE) {
                npv_buffer_add(NPV_LAST_CHILD);
                print_node(GET_LHS(ni));
                npv_buffer_pop();
            }

            break;
        }
        case NT_DATA_VARIANT: {
            printf(" Ident= ");
            print_token_value(tok);

            printf(">\n");

            const NodeIndex extra_start= GET_LHS(ni);
            const NodeIndex extra_end= GET_RHS(ni);
            const NodeIndex default_value= extra_end;

            const bool has_default= default_value != NODE_INDEX_NONE;

            for (size_t i= extra_start; i < extra_end; ++i) {
                if (!has_default && i == extra_end - 1) {
                    npv_buffer_add(NPV_LAST_CHILD);
                } else {
                    npv_buffer_add(NPV_CHILD);
                }
                print_node(NodeIndex_array_get(&extra_array, i));
                npv_buffer_pop();
            }

            if (default_value != NODE_INDEX_NONE) {
                npv_buffer_add(NPV_LAST_CHILD);
                print_node(NodeIndex_array_get(&extra_array, default_value));
                npv_buffer_pop();
            }

            break;
        }
        case NT_TYPE_MARKED:
            break;
        case NT_EXPR_LITERAL: {
            printf(">\n");
        }
            break;
        case NT_EXPR_OPERATOR_BINARY: {
            printf(">\n");

            const NodeIndex lhs= GET_LHS(ni);
            const NodeIndex rhs= GET_RHS(ni);

            npv_buffer_add(NPV_CHILD);
            print_node(lhs);
            npv_buffer_pop();

            npv_buffer_add(NPV_LAST_CHILD);
            print_node(rhs);
            npv_buffer_pop();

            break;
        }
        case NT_EXPR_OPERATOR_UNARY: {
            printf(">\n");

            const NodeIndex lhs= GET_LHS(ni);

            npv_buffer_add(NPV_LAST_CHILD);
            print_node(lhs);
            npv_buffer_pop();

            break;
        }
        case NT_EXPR_VARIABLE: {
            printf(">\n");
            break;
        }
        case NT_EXPR_FUNC_CALL:
            break;
        case NT_EXPR_ARRAY_INDEX:
            break;
        case NT_EXPR_RETURN:
            break;
        case NT_EXPR_BREAK:
            break;
        case NT_EXPR_CONTINUE:
            break;
    }
}
