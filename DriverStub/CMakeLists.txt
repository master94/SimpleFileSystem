project(DriverStub)

file(GLOB SRC_LIST
    "*.h"
    "*.c"
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
