cmake_minimum_required (VERSION 2.8.11)
project (unsharepp)


add_definitions(-Wall)
add_definitions(-Wextra -Werror)
add_definitions(-ggdb3)
add_definitions(-pedantic)
add_definitions(-D_GNU_SOURCE)

add_executable(unsharepp unsharepp.c)
