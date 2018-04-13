cmake_minimum_required(VERSION 2.8)
project(lobby)

include_directories(${CMAKE_BINARY_DIR}/libs/include)
link_directories(${CMAKE_BINARY_DIR}/libs/lib64)

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
    broker.c
)
add_dependencies(broker
    libnanomsg
)
target_link_libraries(broker
    net
    system
    nanomsg
    pthread
    anl
)

add_executable(server
    server.c
)
target_link_libraries(server
    connection
    net
    nanomsg
    pthread
    anl
)

add_executable(client
    client.c
)
target_link_libraries(client
    connection
    net
    nanomsg
    pthread
    anl
)
