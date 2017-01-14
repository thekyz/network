cmake_minimum_required(VERSION 2.6)
project(lobby)

add_library(net
    net.c
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
    nanomsg
    )

add_executable(conn
    connection.c
    )

target_link_libraries(conn
    net
    nanomsg
    )

