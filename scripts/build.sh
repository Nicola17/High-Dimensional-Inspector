#!/bin/bash
#Install the dependencies and build the library in ./build
export C_INCLUDE_PATH=/usr/local/include
export CPLUS_INCLUDE_PATH=/usr/local/include

cd build
cmake  -DCMAKE_BUILD_TYPE=Release ..
make -j 8
