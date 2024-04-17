#!/bin/bash

#	Usage:
# 	compile.sh [-c] [-d] android|native [install|docinstall|...]
# 	-c : clean build directory
#	-d : all of targets use debug mode
#
#	export CMAKE_COMMAND_COMMON=""
# 	export INSTALL_PARENT_DIR=""

set -e

CMAKE_BIN=cmake
# NUM_CORES=$(nproc --all)
NUM_CORES=$(sysctl -n hw.ncpu)

# parse options
DEBUG_MODE=0
CLEAN_BUILD_DIR=0

while getopts 'cd' OPT; do
	case $OPT in
	c)
		echo "CLEAN BUILD DIR!!!"
		CLEAN_BUILD_DIR=1
		;;
	d)
		echo "DEBUG MODE OPEN!!!"
		DEBUG_MODE=1
		;;
	?)
		echo "run.sh options error!"
		exit 1
		;;
	esac
done

shift $(($OPTIND - 1))

PROJECT_FULL_DIR=$(pwd)
PROJECT_DIR=.
SYSTEM=$1
SUB_SYSTEM=$1
if [ "${SUB_SYSTEM}" == "android64" ] || [ "${SUB_SYSTEM}" == "android32" ]; then
	SYSTEM="android"
fi
COMMAND=$2

echo "SYSTEM=${SYSTEM}"
echo "SUB_SYSTEM=${SUB_SYSTEM}"
echo "COMMAND=${COMMAND}"
echo "NDK_HOME=${NDK_HOME}"

if [ "${SYSTEM}" == "android" ]; then
	BUILD_FULL_DIR=${PROJECT_FULL_DIR}/build_android
	BUILD_DIR=${PROJECT_DIR}/build_android
	INSTALL_SUB_DIR=android
else
	BUILD_FULL_DIR=${PROJECT_FULL_DIR}/build_native
	BUILD_DIR=${PROJECT_DIR}/build_native
	INSTALL_SUB_DIR=native
fi

if [ "${INSTALL_PARENT_DIR}" == "" ]; then
	INSTALL_PARENT_DIR=${BUILD_DIR}
fi

INSTALL_DIR="${INSTALL_PARENT_DIR}/${INSTALL_SUB_DIR}"

if [ "${CLEAN_BUILD_DIR}" == "1" ]; then
	rm -rf ${BUILD_DIR}
fi

OUTPUTPATH="arm64-v8a"
if [ "${SUB_SYSTEM}" == "android32" ]; then
	OUTPUTPATH="armeabi-v7a"
else
	OUTPUTPATH="arm64-v8a"
fi

echo "INSTALL_DIR=${INSTALL_DIR}"
echo "OUTPUTPATH=${OUTPUTPATH}"
echo "BUILD_DIR=${BUILD_DIR}"

CMAKE_COMMAND_ANDROID="-DCMAKE_TOOLCHAIN_FILE=${NDK_HOME}/build/cmake/android.toolchain.cmake 
                        -DANDROID_PLATFORM=android-26 
                        -DANDROID_STL=c++_static 
                        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=libs/${OUTPUTPATH}"
if [ "${SUB_SYSTEM}" == "android32" ]; then
	CMAKE_COMMAND_ANDROID="${CMAKE_COMMAND_ANDROID} -DANDROID_ABI=armeabi-v7a"
else
	CMAKE_COMMAND_ANDROID="${CMAKE_COMMAND_ANDROID} -DANDROID_ABI=arm64-v8a"
fi

if [ "${SYSTEM}" == "android" ]; then
	CMAKE_COMMAND_COMMON="$CMAKE_COMMAND_COMMON $CMAKE_COMMAND_ANDROID"
elif [ "${SYSTEM}" == "windows" ]; then
	echo $PATH
	CMAKE_COMMAND_COMMON="$CMAKE_COMMAND_COMMON $CMAKE_COMMAND_WINDOWS"
fi

CMAKE_COMMAND_INTERNAL="-DUSE_INTERNAL_API=ON -DUSE_IMU_API=ON -DUSE_EXPERIMENTAL_API=ON -DUSE_GRAYSCALE_CAMERA_API=ON"
CMAKE_COMMAND_BASE_DEBUG="${CMAKE_BIN} -H${PROJECT_DIR} -B${BUILD_DIR} ${CMAKE_COMMAND_COMMON} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug ${CMAKE_COMMAND_INTERNAL} "
CMAKE_COMMAND_BASE_RELEASE="${CMAKE_BIN} -H${PROJECT_DIR} -B${BUILD_DIR} ${CMAKE_COMMAND_COMMON} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Release "

if [ "${DEBUG_MODE}" == "1" ]; then
	CMAKE_COMMAND_BASE_RELEASE=${CMAKE_COMMAND_BASE_DEBUG}
fi

echo "CMAKE_COMMAND_BASE_RELEASE=${CMAKE_COMMAND_BASE_RELEASE}"

BUILD_COMMAND="$CMAKE_BIN --build ${BUILD_DIR} --parallel ${NUM_CORES}"
INSTALL_COMMAND="$CMAKE_BIN --build ${BUILD_DIR} --parallel ${NUM_CORES} --target install/fast"
BUILD_TARGET_COMMAND="$CMAKE_BIN --build ${BUILD_DIR} --parallel ${NUM_CORES} --target "

runBuild() {
	CMAKE_COMMAND="${CMAKE_COMMAND_BASE_RELEASE}"
	eval ${CMAKE_COMMAND}
	eval ${BUILD_COMMAND}
}

runInstall() {
	CMAKE_COMMAND="${CMAKE_COMMAND_BASE_RELEASE} -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} || exit 1"
	BUILD_COMMAND="${BUILD_COMMAND} || exit 1"
	eval ${CMAKE_COMMAND}
	eval ${BUILD_COMMAND}
	eval ${INSTALL_COMMAND}
}

$CMAKE_BIN --version
case "${COMMAND}" in
"install")
	runInstall
	;;
"docinstall")
	runDocInstall
	;;
"format")
	#centos require: scl enable llvm-toolset-7 bash/zsh
	runFormat
	;;
"app")
	runApps
	;;
*)
	runBuild
	exit 1
	;;
esac
