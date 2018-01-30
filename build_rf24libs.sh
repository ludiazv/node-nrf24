#!/bin/bash
RF24GIT=https://github.com/tmrh20
ALTGIT=https://github.com/ludiazv


echo "Check if RF24libs are installed..."
ldconfig -p | grep librf24.so
if [ $? -eq 0 ]; then
  read -p "Library seems to be installed. Do you want to rebuild last master version? [Y/n]" choice
  case "$choice" in
    n|N ) exit 0 ;;
    * ) echo "-----";;
  esac
fi

echo "Building RF24 libs supported..."
mkdir -p rf24libs
cd rf24libs
if [ -d RF24 ] ; then
cd RF24 ; git pull ; cd ..
else
git clone $RF24GIT/RF24.git RF24
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
