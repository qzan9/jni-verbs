#ifndef _JNIVERBS_H_
#define _JNIVERBS_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

JNIEXPORT jint JNICALL JNI_Onload  (JavaVM *, void *);
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *, void *);

JNIEXPORT jlong JNICALL ibvGetDeviceList(JNIEnv *, jobject, jobject);
JNIEXPORT void JNICALL ibvFreeDeviceList(JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _JNIVERBS_H_ */

