set(SRC_KER CoKernel.cpp Core.cpp FDICU.cpp FileWQ.cpp ThreadCore.cpp TimeWQ.cpp)
add_library(CoKernel ${SRC_KER})
target_include_directories(CoKernel PUBLIC ./ PUBLIC ../CoRo PUBLIC ../utils)
target_link_libraries(CoKernel PUBLIC pthread)