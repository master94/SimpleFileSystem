project(FileSystem)

add_subdirectory(DriverStub)

file(GLOB SRC_LIST
    "*.h"
    "*.c"
)

include_directories(DriverStub)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} DriverStub)
