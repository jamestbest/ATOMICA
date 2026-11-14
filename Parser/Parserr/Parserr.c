//
// Created by jamescoward on 23/10/2026.
//

#include "Parserr.h"

#include <stdarg.h>
#include <stdio.h>

#include "Lexer/Token.h"
#include "Logger/Logger.h"

NodeRes parse_error(const char* message, ...) {
    va_list args;
    va_start(args, message);

    logger_vfadd(message, args);

    va_end(args);

    return NODE_FAIL(EC_FAIL);
}

NodeRes unexpected(MyTokenType expected_type, Token received, char* message, ...) {
    logger_fadd("Expected token type '%s' but received: ", token_type_string(expected_type));
    PRINT_TOKEN(received);
    logger_fadd(" In context: ");

    va_list args;
    va_start(args, message);

    logger_vfadd(message, args);

    va_end(args);

    return NODE_FAIL(EC_FAIL);
}
