cmake_minimum_required(VERSION 3.10)

project(CoEpoll VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

add_executable(mian main.cpp)
add_executable(client client.cpp)
add_executable(ser ser.cpp)
add_subdirectory(CoLib)
target_link_libraries(mian PRIVATE CoLib)
target_link_libraries(ser PRIVATE CoLib)