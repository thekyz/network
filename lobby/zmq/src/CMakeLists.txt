cmake_minimum_required(VERSION 2.6)
project(lobby)

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
target_link_libraries(broker
    net
    system
    zmq
)

add_executable(server
    server.c
)
target_link_libraries(server
    connection
    net
    zmq
)

add_executable(client
    client.c
)
target_link_libraries(client
    connection
    net
    zmq
)

