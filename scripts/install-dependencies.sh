#!/bin/bash
#Install the dependencies and build the library in ./build
sudo apt-get install build-essential libgl1-mesa-dev
sudo add-apt-repository ppa:beineri/opt-qt551-trusty -y
sudo apt update
sudo apt install qt55base qt55-meta-full -y

sudo mkdir /etc/xdg/qtchooser
sudo touch /etc/xdg/qtchooser/default.conf
printf "/opt/qt55/bin\n/opt/qt55/lib" | sudo tee /etc/xdg/qtchooser/default.conf

sudo apt-get install libflann-dev
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
