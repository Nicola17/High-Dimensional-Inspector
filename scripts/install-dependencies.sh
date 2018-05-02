#!/bin/bash
#Install the dependencies and build the library in ./build
sudo apt-get install qtbase5-dev libqt5webkit5-dev libflann-dev
git clone https://github.com/RoaringBitmap/CRoaring.git
cd CRoaring

mkdir -p build
cd build
cmake  -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

cd ../ #build
cd ../ #CRoaring
rm -drf CRoaring
