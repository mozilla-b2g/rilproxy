#!/bin/sh

# http://opekar.blogspot.com/2011/06/android-cmake-is-much-easier-in-ndk-r5b.html

#export NDK=/home/kmachulis/code/mozbuild/android-ndk-r6b/
#export GONK=/home/kmachulis/code/mozbuild/B2G/glue/gonk/
export NDK=$ANDROID_NDK
export GONK=/home/ferjm/dev/B2G/glue/gonk/

SYSROOT=$NDK/platforms/android-9/arch-arm

MIDDLE=toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/
PREF=arm-linux-androideabi-

export CC="$NDK/$MIDDLE/${PREF}gcc  --sysroot=$SYSROOT"
export CXX="$NDK/$MIDDLE/${PREF}g++  --sysroot=$SYSROOT"
export LD="$NDK/$MIDDLE/${PREF}ld  --sysroot=$SYSROOT"
export CPP="$NDK/$MIDDLE/${PREF}cpp  --sysroot=$SYSROOT"
export AS="$NDK/$MIDDLE/${PREF}as  --sysroot=$SYSROOT"
export OBJCOPY="$NDK/$MIDDLE/${PREF}objcopy  --sysroot=$SYSROOT"
export OBJDUMP="$NDK/$MIDDLE/${PREF}objdump  --sysroot=$SYSROOT"
export STRIP="$NDK/$MIDDLE/${PREF}strip  --sysroot=$SYSROOT"
export RANLIB="$NDK/$MIDDLE/${PREF}ranlib  --sysroot=$SYSROOT"
export CCLD="$NDK/$MIDDLE/${PREF}gcc  --sysroot=$SYSROOT"
export AR="$NDK/$MIDDLE/${PREF}ar  --sysroot=$SYSROOT"
