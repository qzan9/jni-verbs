/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef _JNIRDMA_H_
#define _JNIRDMA_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

JNIEXPORT jint JNICALL JNI_Onload  (JavaVM *, void *);
//JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *, void *);

JNIEXPORT jobject JNICALL rdmaSetUp(JNIEnv *, jobject, jobject);
JNIEXPORT void    JNICALL rdmaWrite(JNIEnv *, jobject);
JNIEXPORT void    JNICALL rdmaFree (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _JNIRDMA_H_ */

