cmake_minimum_required (VERSION 3.7)
project (atomic-wait-barrier VERSION 0.1 LANGUAGES CXX)

find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
