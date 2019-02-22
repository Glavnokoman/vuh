//
// Created by longsky on 2019/2/15.
//

#ifndef VUHANDROID_LOG_H
#define VUHANDROID_LOG_H

#include <android/log.h>

#define LOG_TAG "vuhAndroid"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#endif //VUHANDROID_LOG_H
