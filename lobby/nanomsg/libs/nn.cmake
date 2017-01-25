
set(NN_PREFIX libnanomsg)
set(NN_GITHUB https://github.com/nanomsg/nanomsg)
set(NN_VERSION 1.0.0)

ExternalProject_Add(
    ${NN_PREFIX}
    PREFIX ${NN_PREFIX}
    GIT_REPOSITORY ${NN_GITHUB}
    GIT_TAG ${NN_VERSION}
    CMAKE_ARGS -DNN_STATIC_LIB=ON -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/libs
)

