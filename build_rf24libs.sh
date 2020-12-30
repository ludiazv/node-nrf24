#!/bin/bash
RF24GIT=https://github.com/tmrh20
ALTGIT=https://github.com/ludiazv

RF24_VERSION="v1.3.11"
RF24N_VERSION="v1.0.14"
RF24M_VERSION="v1.1.4"  
RF24G_VERSION="TODO"
RF24_DRIVER=SPIDEV

[ -n "$DRIVER" ] && RF24_DRIVER=$DRIVER

set -e
echo "Buiding nrf24 library versions: RF24:$RF24_VERSION RF24NETWORK:$RF24N_VERSION RF24MESH:$RF24M_VERSION DRIVER:$RF24_DRIVER"

#Libraries are allways rebuild as they require FAILURE_HANDLING enabled to operate
if ! [ -x "$(command -v git)" ]; then
  echo 'Error: git is not installed. git is required for installation of the module' >&2
  exit 1
fi
if ! [ -x "$(command -v sed)" ]; then
  echo 'Error: sed is not installed. sed is required for installation of the module' >&2
  exit 1
fi
if ! [ -x "$(command -v make)" ]; then
  echo 'Error: make is not installed. build-essential pkg is required for installation of the module' >&2
  exit 1
fi
if ! [ -x "$(command -v gcc)" ]; then
  echo 'Error: gcc is not installed. build-essential pkg is required for installation of the module' >&2
  exit 1
fi
if ! [ -x "$(command -v g++)" ]; then
  echo 'Error: g++ is not installed. build-essential pkg is required for installation of the module' >&2
  exit 1
fi

if ! [ -x "$(command -v ar)" ]; then
  echo 'Error: ar is not installed. build-essential pkg is required for installation of the module' >&2
  exit 1
fi

#echo "Check if RF24libs are installed..."
#ldconfig -p | grep librf24.so
#if [ $? -eq 0 ]; then
#  read -p "Library seems to be installed. Do you want to rebuild last master version? [Y/n]" choice
#  case "$choice" in
#    n|N ) exit 0 ;;
#    * ) echo "-----";;
#  esac
#fi

mkdir -p rf24libs
cd rf24libs
mkdir -p include
mkdir -p include/RF24
mkdir -p include/RF24Network
mkdir -p include/RF24Mesh
if [ -d RF24 ] ; then
#cd RF24 ; git pull ; cd ..
rm -rf RF24
fi
#else
git clone $RF24GIT/RF24.git RF24
#fi

echo "=>RF24..."
cd RF24
git checkout ${RF24_VERSION}
git show --oneline -s
#echo "===> Activate failure handling ....."
#sed -i '/#define FAILURE_HANDLING/s/^\s.\/\///g' RF24_config.h && cat RF24_config.h | grep FAILURE
# New versions of nrf24 activate FAILURE_HANDLING by default
echo "===> Building..."
./configure --driver=$RF24_DRIVER --header-dir="../include/RF24" 
make
#sudo make install
printf "\nlibrf24.a: \$(OBJECTS)\n\tar -rcs ../librf24.a $^ \n\n" >> Makefile
make librf24.a
make install-headers
cd ..
rm -rf RF24
echo "==> RF24 cleaned"
echo "=>RF24Network..."
if [ -d RF24Network ] ; then
#cd RF24Network ; git pull ; cd ..
rm -fr RF24Network
fi
#else
git clone $RF24GIT/RF24Network.git RF24Network
#fi
cd RF24Network
git checkout ${RF24N_VERSION}
git show --oneline -s
CPATH="../include" make RF24Network.o
#sudo make install
printf "\nlibrf24net.a: RF24Network.o\n\tar -rcs ../librf24net.a $^ \n\t install -m 0644 *.h ../include/RF24Network\n\n" >> Makefile
make librf24net.a
cd ..
rm -rf RF24Network
echo "==> RF24Network cleaned"
echo "=>RF24Mesh..."
if [ -d RF24Mesh ] ; then
#cd RF24Mesh; git pull ; cd ..
rm -rf RF24Mesh
fi
git clone $RF24GIT/RF24Mesh.git RF24Mesh
cd RF24Mesh
git checkout ${RF24M_VERSION}
git show --oneline -s
CPATH="../include" make RF24Mesh.o
printf "\nlibrf24mesh.a: RF24Mesh.o\n\tar -rcs ../librf24mesh.a $^ \n\t install -m 0644 *.h ../include/RF24Mesh\n\n" >> Makefile
make librf24mesh.a
#sudo make install
cd ..
rm -rf RF24Mesh
echo "==> RF24Mesh cleaned"

#echo "=>RF24Gateway..."
#if [ -d RF24Gateway ] ; then
#cd RF24Gateway; git pull ; cd ..
#else
#git clone $RF24GIT/RF24Gateway.git RF24Gateway
#fi
#cd RF24Gateway
#make
#sudo make install
#cd ..

cd ..
#rm -fr rf24libs
echo "done!"
