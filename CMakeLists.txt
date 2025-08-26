cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD         20)

set(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")

project(demo)

add_subdirectory(APRService)

add_executable(demo demo.cpp)
target_link_libraries(demo APRService)
