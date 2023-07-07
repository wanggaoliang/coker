cmake_minimum_required(VERSION 3.10)

project(CoEpoll VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(mian main.cpp)
add_executable(client client.cpp)
add_subdirectory(CoLib)
target_link_libraries(mian PRIVATE CoLib)