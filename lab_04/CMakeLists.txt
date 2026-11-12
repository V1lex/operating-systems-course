cmake_minimum_required(VERSION 3.20)

project(lab_04_variant_9 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(lab4_parent src/parent.c)
add_executable(lab4_child src/child.c)
