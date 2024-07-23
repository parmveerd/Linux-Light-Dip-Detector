# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(light_sampler VERSION 1.0 DESCRIPTION "Assignment 2" LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)

# Enable address sanitizer
# (Comment this out to make your code faster)
add_compile_options(-fsanitize=address)
add_compile_options(-fsanitize=address)

add_link_options(-fsanitize=address)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)

 