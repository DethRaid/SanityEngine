cmake_minimum_required(VERSION 3.14)
project(SanityEngine)

############################
# Initialize cmake options #
############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(SanityEngine.NET)

add_subdirectory(SanityEngine)
