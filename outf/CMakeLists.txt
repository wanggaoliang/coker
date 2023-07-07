add_library(sock Socket.cpp InetAddress.cpp)
target_include_directories(sock
          INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
          )