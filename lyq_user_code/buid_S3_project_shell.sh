#!/bin/bash

top_dir=`pwd`

function build_uboot()
{
	cd "$top_dir/u-boot/"
	make rk3288_config;
	if [ $? -ne 0 ];then
                exit
        fi
	make -j$1;
	if [ $? -ne 0 ];then
                exit
        fi
	cd $top_dir
}
function build_kernel()
{
	cd "$top_dir/kernel"
	make rockchip_H3000_defconfig
	if [ $? -ne 0 ];then
		exit
	fi
	make rk3288-tb_8846.img -j$1
	if [ $? -ne 0 ];then
                exit
        fi
	cp arch/arm/boot/zImage ../out/target/product/S3/kernel
	cd $top_dir
}
function build_android()
{
	cd "$top_dir"

	export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
	export PATH=$JAVA_HOME/bin:$PATH:/sbin
	export CLASSPATH=.:$JAVA_HOME/lib:$JAVA_HOME/lib/tools.jar

	source build/envsetup.sh
	if [ $? -ne 0 ];then
                exit
        fi
	lunch 8
	if [ $? -ne 0 ];then
                exit
        fi
	make update-api -j$1
	if [ $? -ne 0 ];then
                exit
        fi
	make -j$1
	if [ $? -ne 0 ];then
                exit
        fi
	cp kernel/arch/arm/boot/zImage ./out/target/product/S3/kernel
	./mkimage.sh ota
	if [ $? -ne 0 ];then
                exit
        fi
	
}
if [ $1 == "kernel" ]; then
	echo "Build kernel!"
	build_kernel $2
	./mkimage.sh ota
elif [ $1 == "uboot" ]; then
	echo "Build uboot"
	build_uboot $2
elif [ $1 == "android" ]; then
	echo "Build android"
	build_android $2
elif [ $1 == "all" ]; then
	echo "Build all project"
	build_uboot $2
	build_kernel $2
	build_android $2
else
	echo "usage: ./* kernel 8"
	echo " kernel could be change android/uboot/all; 8 is build number thread"
	echo
fi

