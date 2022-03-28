#!/bin/sh
echo "================================"
echo "nfr24 for node js install script"
echo "================================"

node-gyp-build-test
if [ $? -eq 0 ] ; then
    echo "Prebuild binary found!"
    exit 0
else
    set -e
    echo "Prebuild binary not found... compilation required"
    ./build_rf24libs.sh clean
    node-gyp rebuild
fi
exit 0
