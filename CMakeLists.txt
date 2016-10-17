cmake_minimum_required(VERSION 3.5)

# Set project name
project ("ecsos")

# uses boost library for flat_set container
# note that boost is not required for ECSOS itself
find_package (Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

set (CMAKE_CXX_STANDARD 14)

add_executable(example example.cpp)
