make CROSS_COMPILE=arm-linux-gnueabihf- arch=arm -j4 || exit
INSTALL_MOD_PATH=`pwd`/fakeroot make modules_install || exit

cd fakeroot/lib/modules/ && tar czvf 3.4.113+.tar.gz 3.4.113+/ && cd -
