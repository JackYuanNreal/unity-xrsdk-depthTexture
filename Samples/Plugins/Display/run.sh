#!/bin/bash
export NDK_HOME=/Users/hjyuan/Library/Android/sdk/ndk/23.0.7599858
# ./compile.sh -c android32 install
./compile.sh -c android64 install 
cp ../build/android/arm64/*.so ../../Packages/com.unity.xr.sdk.displaysample/Runtime/android/arm64/