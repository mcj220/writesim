#!/bin/sh

export CXX=/oe-builds/build_eos/onemw/build-brcm97449svms-refboard/tmp/sysroots/x86_64-linux/usr/bin/arm-rdk-linux-gnueabi/arm-rdk-linux-gnueabi-g++
export CXXFLAGS=--sysroot=/oe-builds/build_eos/onemw/build-brcm97449svms-refboard/tmp/sysroots/dcx960-debug
make -B
mv writesim writesim.arm

