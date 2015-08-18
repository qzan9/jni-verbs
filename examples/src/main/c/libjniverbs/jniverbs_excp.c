/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#include <jni.h>
#include "jniverbs_excp.h"

void throwException(JNIEnv *env, const char *cls_name, const char *msg)
{
	jclass cls = (*env)->FindClass(env, cls_name);
	if (cls) (*env)->ThrowNew(env, cls, msg);
	(*env)->DeleteLocalRef(env, cls);
}

jint throwNoClassDefExcp(JNIEnv *env, const char *msg)
{
}

jint throwNullPointerExcp(JNIEnv *env, const char *msg)
{
}

jint throwIllegalArgExcp(JNIEnv *env, const char *msg)
{
}

