cmake_minimum_required(VERSION 2.6)
project(lobby)

set(CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
include_directories(${CMAKE_BINARY_DIR}/libs/include)

find_library(ZMQ_LIB NAMES libzmq-static.a PATHS ${CMAKE_BINARY_DIR}/libs/lib)

add_library(net
    net.c
)

add_library(connection
    connection.c
)

add_library(system
    system.c
)

add_executable(broker
    net.c
    broker.c
)
target_link_libraries(broker
    system
    ${ZMQ_LIB}
    pthread
)

add_executable(server
    server.c
)
target_link_libraries(server
    connection
    net
    ${ZMQ_LIB}
    pthread
)

add_executable(client
    client.c
)
target_link_libraries(client
    connection
    net
    ${ZMQ_LIB}
    pthread
)

# Forces c++ linker
set_target_properties(broker${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(server${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(client${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
