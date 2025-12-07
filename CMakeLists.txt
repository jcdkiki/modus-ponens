cmake_minimum_required(VERSION 3.24)
project(modus-ponens)

add_executable(modus-ponens
    src/main.c
    src/expr.c
    src/parser.c
    src/token.c
)
