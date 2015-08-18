/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <infiniband/verbs.h>

#include <jni.h>
#include "jniverbs.h"
#include "jniverbs_excp.h"

#define TEST_NZ(x,y) do { if ( (x) ) die(y); } while (0)    /* if x is NON-ZERO, error is printed */
#define TEST_Z(x,y)  do { if (!(x) ) die(y); } while (0)    /* if x is ZERO,     error is printed */
#define TEST_N(x,y)  do { if ((x)<0) die(y); } while (0)    /* if x is NEGATIVE, error is printed */

static int die(const char *reason);

static JNINativeMethod methods[] = {
	{  "ibvGetDeviceList", "(Lac/ncic/syssw/azq/JniExamples/MutableInteger;)J", (void *)ibvGetDeviceList  },
	{ "ibvFreeDeviceList", "(J)V",                                              (void *)ibvFreeDeviceList },
	{  "ibvGetDeviceName", "(JI)Ljava/lang/String;",                            (void *)ibvGetDeviceName  },
	{  "ibvGetDeviceGUID", "(JI)J",                                             (void *)ibvGetDeviceGUID  },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	JNIEnv *env = NULL;

//	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
	if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {    // there is no JNI_VERSION_1_7 constant.
		return JNI_ERR;
	}

//	if (env->RegisterNatives(env->FindClass("Lac/ncic/syssw/azq/JniExamples/VerbsNative;"),
	if ((*env)->RegisterNatives(env,
	                            (*env)->FindClass(env, "Lac/ncic/syssw/azq/JniExamples/IBVerbsNative;"),
	                            methods,
	                            sizeof(methods) / sizeof(methods[0])
	                           ) < -1) {
		return JNI_ERR;
	}

//	TEST_NZ(ibv_fork_init(), "ibv_fork_init failed.");

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}

JNIEXPORT jlong JNICALL ibvGetDeviceList(JNIEnv *env, jobject this, jobject dev_num)
{
	struct ibv_device **dev_list;
	int num_devices;

	jclass cls;
	jmethodID m_id;

	if (dev_num) {
		TEST_Z(dev_list = ibv_get_device_list(&num_devices), "no IB-device available. get_device_list returned NULL.");
		cls = (*env)->GetObjectClass(env, dev_num);
		m_id = (*env)->GetMethodID(env, cls, "set", "(J)V");
		(*env)->CallVoidMethod(env, dev_num, m_id, num_devices);
	} else {
		throwException(env, "Ljava/lang/IllegalArgumentException;", "argument dev_num shouldn't be null!");
	}

	return (jlong) dev_list;
}

JNIEXPORT void JNICALL ibvFreeDeviceList(JNIEnv *env, jobject this, jlong dev_list)
{
	ibv_free_device_list((struct ibv_device **)dev_list);
}

JNIEXPORT jstring JNICALL ibvGetDeviceName(JNIEnv *env, jobject this, jlong dev_list, jint dev_id)
{
	const char *dev_name = ibv_get_device_name(((struct ibv_device **)dev_list)[dev_id]);
	return (*env)->NewStringUTF(env, dev_name);
}

JNIEXPORT jlong JNICALL ibvGetDeviceGUID(JNIEnv *env, jobject this, jlong dev_list, jint dev_id)
{
	return ibv_get_device_guid(((struct ibv_device **)dev_list)[dev_id]);
}

static int die(const char *reason)
{
	fprintf(stderr, "error: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

