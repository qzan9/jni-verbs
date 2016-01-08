/*
 * Copyright (c) 2015  AZQ
 */

#ifdef __GNUC__
#	define _SVID_SOURCE
#endif /* __GNUC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <jni.h>

#include <my_rdma.h>
#include <chk_err.h>

#include "jnirdma.h"

static int parse_user_config(JNIEnv *, jobject, struct user_config *);
static void throwException(JNIEnv *, const char *, const char *);

static const JNINativeMethod methods[] = {
	{  "rdmaInit",      "(Lac/ncic/syssw/jni/RdmaUserConfig;)Ljava/nio/ByteBuffer;", (void *)rdmaInit       },
	{ "rdmaWrite",      "()V",                                                       (void *)rdmaWrite      },
	{ "rdmaWriteAsync", "(II)V",                                                     (void *)rdmaWriteAsync },
	{ "rdmaPollCq",     "(I)V",                                                      (void *)rdmaWriteAsync },
	{  "rdmaFree",      "()V",                                                       (void *)rdmaFree       },
};

static struct rdma_context *rctx;
static struct user_config  *ucfg;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	JNIEnv *env = NULL;

//	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
	if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {    // there is no JNI_VERSION_1_7 constant.
		return JNI_ERR;
	}

//	if (env->RegisterNatives(env->FindClass("Lac/ncic/syssw/jni/JniRdma;"),
	if ((*env)->RegisterNatives(env,
	                            (*env)->FindClass(env, "Lac/ncic/syssw/jni/JniRdma;"),
	                            methods,
	                            sizeof(methods) / sizeof(methods[0])
	                           ) < -1) {
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}

//JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
//{
//}

JNIEXPORT jobject JNICALL rdmaInit(JNIEnv *env, jobject thisObj, jobject userConfig)
{
	if (!(rctx = malloc(sizeof *rctx))) {
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to allocate rdma context!");
		return NULL;
	}
	if (!(ucfg = malloc(sizeof *ucfg))) {
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to allocate user config!");
		return NULL;
	}
	memset(rctx, 0, sizeof *rctx);
	memset(ucfg, 0, sizeof *ucfg);

	srand48(getpid() * time(NULL));

	if (parse_user_config(env, userConfig, ucfg)) {
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to parse user config!");
		return NULL;
	}

	if (init_context(rctx, ucfg)) {
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to set up RDMA context!");
		return NULL;
	}

	if (connect_to_peer(rctx, ucfg)) {
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to make IB connection!");
		return NULL;
	}

	return (*env)->NewDirectByteBuffer(env, rctx->buf, rctx->size*2);
}

JNIEXPORT void JNICALL rdmaWrite(JNIEnv *env, jobject thisObj)
{
	if (rdma_write(rctx))
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to perform RDMA write operation!");
	return;
}

JNIEXPORT void JNICALL rdmaWriteAsync(JNIEnv *env, jobject thisObj, jint offset, jint length)
{
	if (rdma_write(rctx), offset, length)
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to perform asynchronous RDMA write operation!");
	return;
}

JNIEXPORT void JNICALL rdmaPollCq(JNIEnv *env, jobject thisObj, jint num_entries)
{
	if (rdma_poll_cq(rctx, num_entries))
		throwException(env, "Lac/ncic/syssw/jni/RdmaException;", "failed to poll CQ entries!");
	return;
}

JNIEXPORT void JNICALL rdmaFree(JNIEnv *env, jobject thisObj)
{
	if (ucfg->server_name)
		(*env)->ReleaseStringUTFChars(env, NULL, ucfg->server_name);
	free(ucfg);

	if (destroy_context(rctx)) {
		fprintf(stderr, "failed to destroy RDMA context!\n");
		free(rctx);
		exit(EXIT_FAILURE);
	}
	free(rctx);

	return;
}

static int parse_user_config(JNIEnv *env, jobject userConfig, struct user_config *ucfg)
{
	jclass userConfigCls;
	jmethodID userConfigMethodId;

	CHK_ZPI(userConfig, "user config is NULL!");

	CHK_ZI(userConfigCls = (*env)->GetObjectClass(env, userConfig));

	ucfg->ib_dev  = 0;    // note: pick your properly working IB device
	ucfg->ib_port = 1;    //       and port.

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getBufferSize", "()I"));
	ucfg->buffer_size = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getSocketPort", "()I"));
	ucfg->sock_port = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getServerName", "()Ljava/lang/String;"));
	jstring serverName = (jstring)(*env)->CallObjectMethod(env, userConfig, userConfigMethodId);
	if (serverName) {
		ucfg->server_name = (char *)(*env)->GetStringUTFChars(env, serverName, NULL);
			// NOTE: instance pinned, copy allocated -> need be freed.
			// here's also buggy: if GetStringUTFChars returns NULL, the program would consider herself as the server.
	}
	(*env)->ReleaseStringUTFChars(env, serverName, NULL);    // not sure whether or not this would work.

	(*env)->DeleteLocalRef(env, userConfigCls);

	if ((*env)->ExceptionOccurred(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}

	return 0;
}

static void throwException(JNIEnv *env, const char *cls_name, const char *msg)
{
	jclass cls = (*env)->FindClass(env, cls_name);
	if (cls) (*env)->ThrowNew(env, cls, msg);
	(*env)->DeleteLocalRef(env, cls);

	return;
}

