cmake_minimum_required(VERSION 4.2.2)
project(ATOMICA C)

set(CMAKE_C_STANDARD 99)

include_directories(
    .
)

#add_link_options(-fsanitize=undefined)
#add_compile_options(-fsanitize=undefined)

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
        Error.c
        Parser/TLSCollection/TLSPass.c
        Parser/TLSCollection/TLSPass.h
        Gluon/Jobs/Job.c
        Gluon/Jobs/Job.h
        shared/QueueBlocking.c
        shared/QueueBlocking.h
        shared/ArrayG.c
        shared/ArrayG.h
        Gluon/Gluon.c
        Gluon/Gluon.h)


add_executable(EXAMPLES_300
        atom_examples/300_global_static_scope/Lexer.c
        atom_examples/300_global_static_scope/Parser.c
        atom_examples/300_global_static_scope/main.c
)
