cmake_minimum_required(VERSION 3.18)
project(LED VERSION 1.0 DESCRIPTION "LED project" LANGUAGES C)


set(CMAKE_C_STANDARD 23)
# add_compile_options(-Wall -Werror -Wpendatic -Wextra)
# add_compile_options(-fdiagonstics-color)

add_subdirectory(hal)
add_subdirectory(app)