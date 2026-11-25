cmake_minimum_required(VERSION 3.20)

project(lab_05_variant_20 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(lab5_first SHARED src/lib1.c)
add_library(lab5_second SHARED src/lib2.c)

add_executable(lab5_linked src/program1.c)
target_link_libraries(lab5_linked PRIVATE lab5_first)

add_executable(lab5_dynamic src/program2.c)
target_link_libraries(lab5_dynamic PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(lab5_dynamic lab5_first lab5_second)