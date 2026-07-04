#pragma once
#include <jni.h>

namespace pairipfix {
bool IsCandidatePackage(const char* nicePackage);
bool HasPairip(JNIEnv* env, jobject appClassLoader);
}
