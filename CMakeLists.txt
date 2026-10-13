cmake_minimum_required(VERSION 4.3)
project(ATOMICA C)

set(CMAKE_C_STANDARD 99)

add_executable(ATOMICA main.c)


add_executable(EXAMPLES_300
        atom_examples/300_global_static_scope/Lexer.c
        atom_examples/300_global_static_scope/Parser.c
        atom_examples/300_global_static_scope/main.c
)
