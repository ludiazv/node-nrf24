#! /bin/bash
echo "Docker build assitant for travis..."

set -ex
DOCKER_NODE="arm32v7/node:10-buster"
[ -n "$1" ] && DOCKER_NODE="$1"

echo "Building with docker for node $DOCKER_NODE"
#docker run --rm --privileged multiarch/qemu-user-static:register --reset -p yes
#docker run -t --rm -v $(pwd):/root/node-nrf24 \
#           --workdir /root/node-nrf24 \
#           $DOCKER_NODE \
#           /bin/bash -c "npm install node-gyp -g && ./build_rf24libs.sh && node-gyp rebuild"
#            /bin/bash -c "node --version"
docker run -t --rm --entrypoint="/bin/sh" -v$(pwd):/root/app --workdir /root/app $NODE_VERSION \
       -c "npm --version && npm install && npm install node-gyp -g && ./build_rf24libs.sh && node-gyp rebuild"

echo "Finished with: $?"
echo "done!" 
