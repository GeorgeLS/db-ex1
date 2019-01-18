cmake_minimum_required(VERSION 3.13)
project(db_ex1 C)

set(CMAKE_C_STANDARD 11)

add_executable(db_ex1
        ht_main_test.c Source/HT.c Include/macros.h
        Include/attributes.h
        Include/record.h Source/record.c)

target_link_libraries(db_ex1 ${CMAKE_SOURCE_DIR}/BF_64.a)