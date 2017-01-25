
set(ZMQ_PREFIX libzmq)
set(ZMQ_GITHUB https://github.com/zeromq/libzmq)
set(ZMQ_VERSION v4.2.1)

ExternalProject_Add(
    ${ZMQ_PREFIX}
    PREFIX ${ZMQ_PREFIX}
    GIT_REPOSITORY ${ZMQ_GITHUB}
    GIT_TAG ${ZMQ_VERSION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/libs
)

