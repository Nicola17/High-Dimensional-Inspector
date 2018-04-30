#!/bin/bash
#Install the dependencies and build the library in ./build
apt-get install qtbase5-dev libqt5webkit5-dev libflann-dev
rm -drf build
mkdir build
cd build
cmake ../
make -j 8
