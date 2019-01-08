#!/bin/bash 

#build kernel
cd kernel
make rv1108_ipc_defconfig
make rv1108-campen-v10.img -j8 2 >&1 | tee build_0.log
echo "lyq:make rv1108-campen-v10.img"

#build others
cd ..
source config/envsetup.sh
./build_all.sh -j8 2>&1	| tee build_1.log
echo "lyq:build_all loctaion"

#package
./mkfirmware.sh rv1108-campen-v10 2 >&1 | tee build_2.log
