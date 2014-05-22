project(FileSystem)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(./driver DRIVER_DIR)
file(GLOB SRC_LIST
    "*.h"
    "*.c"
)
add_executable(${PROJECT_NAME} ${SRC_LIST})
include_directories(./driver)
