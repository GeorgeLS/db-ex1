cmake_minimum_required(VERSION 3.13)
project(db_ex1 C)

set(CMAKE_C_STANDARD 11)

add_executable(db_ex1 Source/main.c Source/HT.c Include/macros.h Include/attributes.h)