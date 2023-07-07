set(SRC_KER Component.cpp ICU.cpp IRQPoll.cpp CPU.cpp CompStarter.cpp)
add_library(Component ${SRC_KER})
target_include_directories(Component PUBLIC ./ PUBLIC ../../CoRo PUBLIC ../../utils ../irq)
target_link_libraries(Component PUBLIC pthread)