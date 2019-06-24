#!/bin/bash

build_modules()
{
    # ENV
    make CROSS_COMPILE=arm-linux-gnueabihf- arch=arm -j4 || exit -1

    # Install zImage to fakeroot~
    cp `pwd`/arch/arm/boot/zImage `pwd`/fakeroot/boot/

    # Prepare
    mkdir -p fakeroot/lib/modules

    # Build and install modules
    INSTALL_MOD_PATH=`pwd`/fakeroot make modules_install || exit -1

    # Package modules
    cd fakeroot/lib/modules/ && {
        tar czvf 3.4.113+.tar.gz 3.4.113+/ && cd -
        exit 0
    }
}

patch_src()
{
    git diff armbian-patched > z-1000-QPT-kernel-3.4.11.patch
}

usage()
{
    echo "./make.sh <build> --- 编译打包内核&模块, 可拷贝到目标设备调试"
    echo "          <patch> --- 生成armbian补丁"
}

case $1 in
    patch)
        echo "Patch code modify from armbian-original."
        patch_src
    ;;
    build)
        echo "Build kernel and modules."
        build_modules
    ;;
    *)
        usage
    ;;
esac
