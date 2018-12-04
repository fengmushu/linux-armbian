make CROSS_COMPILE=arm-linux-gnueabihf- arch=arm -j4
INSTALL_MOD_PATH=`pwd`/fakeroot make modules_install
