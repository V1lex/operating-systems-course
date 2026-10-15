cmake_minimum_required(VERSION 3.20)

project(lab_03_variant_3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

find_package(Threads REQUIRED)
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(lab3_merge_sort src/merge_sort.c)
target_link_libraries(lab3_merge_sort PRIVATE Threads::Threads)
