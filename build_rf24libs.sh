#!/bin/bash
RF24GIT=https://github.com/tmrh20
ALTGIT=https://github.com/ludiazv

echo "Building RF24 libs supported..."
mkdir -p rf24libs
cd rf24libs
if [ -d RF24 ] ; then
cd RF24 ; git pull ; cd ..
else
git clone $ALTGIT/RF24.git RF24
fi
echo "=>RF24..."
cd RF24
./configure --driver=SPIDEV
make
sudo make install
cd ..
echo "=>RF24Network..."
if [ -d RF24Network ] ; then
cd RF24Network ; git pull ; cd ..
else
git clone $RF24GIT/RF24Network.git RF24Network
fi
cd RF24Network
make
sudo make install
cd ..
echo "=>RF24Mesh..."
if [ -d RF24Mesh ] ; then
cd RF24Mesh; git pull ; cd ..
else
git clone $RF24GIT/RF24Mesh.git RF24Mesh
fi
cd RF24Mesh
make
sudo make install
cd ..
echo "=>RF24Gateway..."
if [ -d RF24Gateway ] ; then
cd RF24Gateway; git pull ; cd ..
else
git clone $RF24GIT/RF24Gateway.git RF24Gateway
fi
cd RF24Gateway
make
sudo make install
cd ..
cd ..
echo "done!"
