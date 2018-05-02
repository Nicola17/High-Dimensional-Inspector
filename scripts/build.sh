#!/bin/bash
#Install the dependencies and build the library in ./build
export C_INCLUDE_PATH=/usr/local/include
export CPLUS_INCLUDE_PATH=/usr/local/include

cd build
cmake ../
make -j 8
