cmake_minimum_required(VERSION 4.2.2)
project(ATOMICA C)

set(CMAKE_C_STANDARD 99)

include_directories(
    .
)

add_link_options(-fsanitize=undefined)
add_compile_options(-fsanitize=undefined)

add_executable(ATOMICA main.c
        Lexer/Token.c
        Lexer/Token.h
        Lexer/Lexer.c
        Lexer/Lexer.h
        shared/Array.c
        shared/Array.h
        Parser/Parser.c
        Parser/Parser.h
        shared/Math.h
        Parser/Parserr/Parserr.c
        Parser/Parserr/Parserr.h
        Parser/Shunt/ShuntingYard.c
        Parser/Shunt/ShuntingYard.h
        Parser/SharedParsing.h
        shared/String.c
        shared/String.h
        Parser/Node.c
        Parser/Node.h
        Error.h
        Error.c)


add_executable(EXAMPLES_300
        atom_examples/300_global_static_scope/Lexer.c
        atom_examples/300_global_static_scope/Parser.c
        atom_examples/300_global_static_scope/main.c
)
