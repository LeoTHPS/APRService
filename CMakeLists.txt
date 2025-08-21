cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD           17)
set(CMAKE_CXX_STANDARD         20)

set(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

project(demo)

add_subdirectory(APRService)

add_executable(demo demo.cpp)
target_link_libraries(demo APRService)
