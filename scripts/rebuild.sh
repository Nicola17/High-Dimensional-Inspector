#!/bin/bash
#Install the dependencies and build the library in ./build
rm -drf build
mkdir build
cd build
cmake ../
make -j 8
