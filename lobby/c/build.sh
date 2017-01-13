#!/bin/bash

clang -g -c net.c -o net.o
clang -g net.o connection.c -o conn -lnanomsg
clang -g net.o broker.c -o broker -lnanomsg

