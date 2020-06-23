#!/bin/bash

HOST_DIR="$PWD"
BUILD_DIR=$HOST_DIR/build

CopyRsrc()
{
	cp -r $HOST_DIR/example/data $BUILD_DIR
}

CompileShaders()
{
	pushd $HOST_DIR/build/data/shaders
		rm -f *.spv

		for file in *; do
    		if [ -f "$file" ]; then
        		echo "$file"
				glslc $file -o $file.spv
    		fi
		done
	popd
}

#### Main

if [ "$1" == "rsrc" ];
then
	CopyRsrc
fi

if [ "$1" == "build_shad" ];
then
	CompileShaders
fi

if [ "$1" == "gdb" ];
then
	pushd $BUILD_DIR
		gdb ./maple.exe
	popd
fi

if [ "$1" == "val" ];
then
	pushd $BUILD_DIR
		valgrind ./maple.exe
	popd
fi

if [ "$1" == "" ];
then
	pushd $BUILD_DIR
		./maple.exe
	popd
fi
