following https://developer.android.google.cn/ndk/guides/graphics/shader-compilers
	https://developer.android.com/ndk/guides/graphics/validation-layer?hl=zh-cn
	
saxpy example on Android

```
$ cd ndk_root/sources/third_party/shaderc/

$ ../../../ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk \
APP_STL:=c++_static APP_ABI=all libshaderc_combined

$ ../../../ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk \
APP_STL:=c++_shared APP_ABI=all libshaderc_combined

```
Android Studio 3.3.1

NDK 19.1.5304403

We need cmake 3.10.2 

```
$ export ANDROID_SDK_HOME=~/Library/Android/sdk/

$ ${ANDROID_SDK_HOME}/tools/bin/sdkmanager --update

$ export ANDROID_CMAKE_REV_3_10="3.10.2.4988404"

$ ${ANDROID_SDK_HOME}/tools/bin/sdkmanager "cmake;$ANDROID_CMAKE_REV_3_10"

```

macOS Mojave ,if ${ANDROID_SDK_HOME}/tools/bin/sdkmanager return error, maybe it's java's version incorrect ,use following command install java

```
$ brew cask install java8
 
$ export JAVA_HOME=`/usr/libexec/java_home -v 1.8`

$ export ANDROID_SDK_HOME=~/Library/Android/sdk/

$ ${ANDROID_SDK_HOME}/tools/bin/sdkmanager --update

$ export ANDROID_CMAKE_REV_3_10="3.10.2.4988404"

$ ${ANDROID_SDK_HOME}/tools/bin/sdkmanager "cmake;$ANDROID_CMAKE_REV_3_10"

```

debug Validation Layers following
    https://developer.android.com/ndk/guides/graphics/validation-layer?hl=zh-cn
