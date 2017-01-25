cmake_minimum_required(VERSION 2.8)
project(lobby_nn)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(ExternalProject)
include(libs/nn.cmake)

add_subdirectory(src)

