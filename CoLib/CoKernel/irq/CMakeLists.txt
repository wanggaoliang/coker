set(SRC_KER FileWQ.cpp TimeWQ.cpp)
add_library(IRQ ${SRC_KER})
target_include_directories(IRQ PUBLIC ./ PUBLIC ../../CoRo PUBLIC ../../utils)