/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef _JNIVERBS_H_
#define _JNIVERBS_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

JNIEXPORT jint JNICALL JNI_Onload  (JavaVM *, void *);
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *, void *);

JNIEXPORT jlong JNICALL ibvGetDeviceList (JNIEnv *, jobject, jobject);
JNIEXPORT void  JNICALL ibvFreeDeviceList(JNIEnv *, jobject, jlong);

JNIEXPORT jstring JNICALL ibvGetDeviceName(JNIEnv *, jobject, jlong, jint);
JNIEXPORT jlong   JNICALL ibvGetDeviceGUID(JNIEnv *, jobject, jlong, jint);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _JNIVERBS_H_ */

