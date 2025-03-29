cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD   17)
set(CMAKE_CXX_STANDARD 20)

project(APRService)

add_subdirectory(APRService)

add_executable(demo demo.cpp)
target_link_libraries(demo APRService)
