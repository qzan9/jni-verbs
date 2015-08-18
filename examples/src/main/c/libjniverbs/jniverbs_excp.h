/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef _JNIVERBS_EXCP_H_
#define _JNIVERBS_EXCP_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void throwException(JNIEnv *, const char *, const char *);

jint throwNoClassDefExcp(JNIEnv *, const char *);
jint throwNullPointerExcp(JNIEnv *, const char *);
jint throwIllegalArgExcp(JNIEnv *, const char *);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _JNIVERBS_EXCP_H_ */

