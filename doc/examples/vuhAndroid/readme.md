following https://developer.android.google.cn/ndk/guides/graphics/shader-compilers
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