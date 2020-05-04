#!/bin/bash

# External library directories
VK_COMPILER="~/vulkan/1.2.135.0/x86_64/bin"
VK_INCLUDE="~/vulkan/1.2.135.0/x86_64/include"
VK_LIB="~/vulkan/1.2.135.0/x86_64/lib"

HOST_DIR="$PWD"
echo Host directory is: $HOST_DIR

INC_DIR=$HOST_DIR/inc
ENGINE_DIR=$HOST_DIR/engine
SRC_DIR=$HOST_DIR/example
EXT_DIR=$HOST_DIR/ext
BUILD_DIR=$HOST_DIR/build

GLFW_INC=
#$EXT_DIR/glfw-3.3.2/include

UNITY_SRC=$HOST_DIR/unity.cpp

CFLAGS="-g -std=c++17 -Wall"

INC="-I$VK_INCLUDE -I$EXT_DIR"

if [ ! -d $BUILD_DIR ];
then
	mkdir build
fi

pushd $BUILD_DIR
	echo Building game...
	echo g++ $CFLAGS -Wno-reorder -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing $INC -DVK_USE_PLATFORM_XLIB_KHR -DVK_NO_PROTOTYPES $UNITY_SRC -o example -ldl -lpthread -lX11 -Wl,-rpath=$VK_LIB
	g++ $CFLAGS -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -fno-strict-aliasing $INC -DVK_USE_PLATFORM_XCB_KHR -DVK_NO_PROTOTYPES $UNITY_SRC -o example -ldl -lpthread -lxcb -Wl,-rpath=$VK_LIB
popd