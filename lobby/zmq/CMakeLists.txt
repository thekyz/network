cmake_minimum_required(VERSION 2.8)
project(lobby_zmq)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(ExternalProject)
include(libs/zmq.cmake)

add_subdirectory(src)

