#! /bin/bash
echo "Docker based cross-compilation script for nrf24 nodejs module"

# Distribution to use for prebuild
PREBUILD_IMAGE="node:current-buster"

# node versions as targets to prebuild
PREBUILD_VERSIONS=( "8.17.0" "9.11.2" "10.23.0" "11.15.0" "12.20.0" "13.14.0" "14.15.3")

# prebuild architectures & configuration
PREBUILD_ARCHS=( "arm32v7" "arm64v8" )
PREBUILD_ARCHS_FLAGS=( "--tag-armv 7" "--tag-armv 8")
PREBUILD_FLAGS="--strip --libc glibc --tag-libc"

function run_in_node() {
       local node_image="$1"
       docker run -t --rm --entrypoint="/bin/sh" -v$(pwd):/root/app --workdir /root/app $node_image -c "${2}"
}

function compile_in_node() {
       run_in_node $1 "node --version && npm --version && npm install node-gyp -g && npm install --ignore-scripts  && ./build_rf24libs.sh clean && node-gyp rebuild"
}

function prebuild() {
       local arch=$1
       local arch_flags=$2
       local targets=""
       for tar in ${PREBUILD_VERSIONS[@]}
       do
              targets="$targets -t ${tar}"
       done
       local cmd="node --version && npm --version && npm install node-gyp prebuildify -g && npm install --ignore-scripts && ./build_rf24libs.sh clean && node-gyp clean"
       cmd="$cmd && prebuildify $targets ${PREBUILD_FLAGS} ${arch_flags}"
       echo "$arch -> $cmd"
       run_in_node "${arch}/${PREBUILD_IMAGE}" "${cmd}"
}


set -ex

if [ "$1" == "build" ] ; then
       DOCKER_NODE="arm32v7/node:12-buster"
       [ -n "$2" ] && DOCKER_NODE="$2"
       echo "Single compile in $DOCKER"
       compile_in_node $DOCKER_NODE
       echo "Compiled ok!"
       exit 0
fi

if [ "$1" == "prebuild" ] ; then
       # Cleaning prebuilds
       rm -fR prebuilds
       if [ -z "$2" ] ; then
              echo "Prebuild all architectures ${PREBUILD_ARCHS[@]}..."
              i=0
              for arch in ${PREBUILD_ARCHS[@]} 
              do     
                     echo "Prebuilding for arch $arch"
                     prebuild $arch ${PREBUILD_ARCHS_FLAGS[$i]}
                     i= $(expr $i + 1) 
              done
              # Be sure to delete all artificats
              run_in_node "${PREBUILD_ARCHS[0]}/${PREBUILD_IMAGE}" "rm -Rf build && rm -Rf rf24libs"
       else 
              echo "Prebuild $2..."
              prebuild ${PREBUILD_ARCHS[${2}]} ${PREBUILD_ARCHS_FLAGS[${2}]}
              run_in_node "${PREBUILD_ARCHS[${2}]}/${PREBUILD_IMAGE}" "rm -Rf build && rm -Rf rf24libs"
       fi
       
       exit 0
fi

echo "Invalid command $1"
echo "usage $0 <cmd>"
echo "         build <arch>/<nodeversion> "
echo "         prebuild [arch]"
echo
exit 1

#echo "Building with docker for node $DOCKER_NODE"
#docker run --rm --privileged multiarch/qemu-user-static:register --reset -p yes
#docker run -t --rm -v $(pwd):/root/node-nrf24 \
#           --workdir /root/node-nrf24 \
#           $DOCKER_NODE \
#           /bin/bash -c "npm install node-gyp -g && ./build_rf24libs.sh && node-gyp rebuild"
#            /bin/bash -c "node --version"
#docker run -t --rm --entrypoint="/bin/sh" -v$(pwd):/root/app --workdir /root/app $NODE_VERSION \
#       -c "npm --version && npm install && npm install node-gyp -g && ./build_rf24libs.sh && node-gyp rebuild"

#echo "Finished with: $?"
#echo "done!" 
