set(SRC_KER ASocket.cpp AFiber.cpp AMutex.cpp)
add_library(CoLib ${SRC_KER})
add_subdirectory(CoKernel)
target_link_libraries(CoLib PUBLIC CoKernel)