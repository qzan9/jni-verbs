/*
 * Copyright (c) 2015  AZQ
 */

#ifndef _JNIRDMA_H_
#define _JNIRDMA_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

JNIEXPORT jint JNICALL JNI_Onload  (JavaVM *, void *);
//JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *, void *);

JNIEXPORT jobject JNICALL rdmaInit (JNIEnv *, jobject, jobject);
JNIEXPORT void    JNICALL rdmaWrite(JNIEnv *, jobject);
JNIEXPORT void    JNICALL rdmaWriteAsync(JNIEnv *, jobject, jint, jint);
JNIEXPORT void    JNICALL rdmaPollCq    (JNIEnv *, jobject, jint);
JNIEXPORT void    JNICALL rdmaFree (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _JNIRDMA_H_ */

