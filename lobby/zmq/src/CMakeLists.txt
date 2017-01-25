cmake_minimum_required(VERSION 2.8)
project(lobby)

set(CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
include_directories(${CMAKE_BINARY_DIR}/libs/include)
link_directories(${CMAKE_BINARY_DIR}/libs/lib)

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
    libzmq
)
target_link_libraries(broker
    net
    system
    zmq-static
    pthread
)

add_executable(server
    server.c
)
target_link_libraries(server
    connection
    net
    zmq-static
    pthread
)

add_executable(client
    client.c
)
target_link_libraries(client
    connection
    net
    zmq-static
    pthread
)

# Forces c++ linker
set_target_properties(broker${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(server${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(client${DEBUGEXT} PROPERTIES LINKER_LANGUAGE CXX)
