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

#define IBV_NZ(ibv,env,msg) do { if ( (ibv) ) bluepill((env),(msg)); } while (0)    /* if x is NON-ZERO, exception is throwed. */
#define IBV_Z(ibv,env,msg)  do { if (!(ibv) ) bluepill((env),(msg)); } while (0)    /* if x is ZERO,     exception is throwed. */
#define IBV_N(ibv,env,msg)  do { if ((ibv)<0) bluepill((env),(msg)); } while (0)    /* if x is NEGATIVE, exception is throwed. */

static int bluepill(JNIEnv *, const char *);

static const JNINativeMethod methods[] = {
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

//	if (env->RegisterNatives(env->FindClass("Lac/ncic/syssw/azq/JniExamples/JniVerbs;"),
	if ((*env)->RegisterNatives(env,
	                            (*env)->FindClass(env, "Lac/ncic/syssw/azq/JniExamples/JniVerbs;"),
	                            methods,
	                            sizeof(methods) / sizeof(methods[0])
	                           ) < -1) {
		return JNI_ERR;
	}

//	IBV_NZ(ibv_fork_init(), env, "ibv_fork_init failed!");

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
		IBV_Z(dev_list = ibv_get_device_list(&num_devices), env, "ibv_get_device_list failed!");
		cls = (*env)->GetObjectClass(env, dev_num);
		m_id = (*env)->GetMethodID(env, cls, "set", "(J)V");
		(*env)->CallVoidMethod(env, dev_num, m_id, num_devices);
	} else {
		throwException(env, "Ljava/lang/IllegalArgumentException;", "ibvGetDeviceList: NULL param!");
	}

	return (jlong) dev_list;
}

JNIEXPORT void JNICALL ibvFreeDeviceList(JNIEnv *env, jobject this, jlong dev_list)
{
	ibv_free_device_list((struct ibv_device **)dev_list);
}

JNIEXPORT jstring JNICALL ibvGetDeviceName(JNIEnv *env, jobject this, jlong dev_list, jint dev_id)
{
	return (*env)->NewStringUTF(env, ibv_get_device_name(((struct ibv_device **)dev_list)[dev_id]));
}

JNIEXPORT jlong JNICALL ibvGetDeviceGUID(JNIEnv *env, jobject this, jlong dev_list, jint dev_id)
{
	return ibv_get_device_guid(((struct ibv_device **)dev_list)[dev_id]);
}

static int bluepill(JNIEnv *env, const char *msg)
{
	fprintf(stderr, "error: %s - %s\n", strerror(errno), msg);
//	exit(EXIT_FAILURE);
	throwException(env, "Lac/ncic/syssw/azq/JniExamples/VerbsException;", msg);
	return -1;
}

