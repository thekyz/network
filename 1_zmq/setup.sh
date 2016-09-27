#!/bin/bash

#sudo apt-get update
#sudo apt-get install python-dev -y

CWD=$(pwd -P)

mkdir zmq_lib
cd zmq_lib
mkdir build
wget https://github.com/zeromq/zeromq4-1/releases/download/v4.1.5/zeromq-4.1.5.tar.gz
tar xzf zeromq-4.1.5.tar.gz
cd zeromq-4.1.5
./configure --prefix=${CWD}/zmq_lib
make
make install

#sudo pip install pyzmq

