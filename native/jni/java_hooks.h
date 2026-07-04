#pragma once
#include <jni.h>

namespace pairipfix {
void InstallJavaHooks(JNIEnv* env, jobject appClassLoader, const char* selfPackage);
void InstallAnchor(JNIEnv* env, void* onCreateFnPtr);
}
