#!/bin/bash

clang -g connection.c -o conn -lnanomsg
clang -g broker.c -o broker -lnanomsg

