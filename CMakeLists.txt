project(FileSystem)
cmake_minimum_required(VERSION 2.8)
file(GLOB SRC_LIST
    "*.h"
    "*.c"
)
add_executable(${PROJECT_NAME} ${SRC_LIST})
