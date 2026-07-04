#pragma once
#include <android/log.h>

#ifndef PAIRIP_LOG
#define PAIRIP_LOG 0
#endif

#define PAIRIP_TAG "PairIPFix"

#if PAIRIP_LOG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, PAIRIP_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, PAIRIP_TAG, __VA_ARGS__)
#else
#define LOGD(...) ((void)0)
#define LOGE(...) ((void)0)
#endif
