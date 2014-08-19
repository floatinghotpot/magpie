
#include <jni.h>
#include "../platform/android/jni/JniHelper.h"

#include "cocos2d.h"
#include "../../Classes/Magpie.h"
#include "MagpieBridgeJni.h"

#include <android/log.h>

#define  LOG_TAG    "MagpieBridge"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

using namespace cocos2d;

extern "C"
{

void MagpieBridge_execute(const char * service, const char * action, const char * argsJson, const char * callbackId)
{
	LOGD("execute: %s, %s, %s, %s", service, action, argsJson, callbackId);

	JniMethodInfo t;
	if(JniHelper::getStaticMethodInfo(t, "com/handywit/magpie/MagpieBridge", "exec", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V")) {
		jstring jservice = t.env->NewStringUTF(service);
		jstring jaction = t.env->NewStringUTF(action);
		jstring jargs = t.env->NewStringUTF(argsJson);
		jstring jcallbackId = t.env->NewStringUTF(callbackId);

		t.env->CallStaticVoidMethod(t.classID, t.methodID, jservice, jaction, jargs, jcallbackId);

		t.env->DeleteLocalRef( jservice );
		t.env->DeleteLocalRef( jaction );
		t.env->DeleteLocalRef( jargs );
		t.env->DeleteLocalRef( jcallbackId );

	} else {
		LOGD("java class method com/handywit/magpie/MagpieBridge not found");
	}
}

void Java_com_handywit_magpie_MagpieBridge_event(JNIEnv *env, jobject thiz, jstring jevent, jstring jargs)
{
	const char * event = env->GetStringUTFChars(jevent, NULL);
	const char * args = env->GetStringUTFChars(jargs, NULL);

	LOGD("event: %s %s", event, args);

	Magpie::instance()->event( event, args );

	env->ReleaseStringUTFChars(jevent, event);
	env->ReleaseStringUTFChars(jargs, args);
}

void Java_com_handywit_magpie_MagpieBridge_callback(JNIEnv *env, jobject thiz,
		jstring jcallbackId, jboolean jsuccess, jint jstatus, jstring jdatatype, jstring jdata, jboolean jkeep)
{
	const char * callbackId = env->GetStringUTFChars(jcallbackId, NULL);
	bool success = (jsuccess == JNI_TRUE);
	int statusCode = (int) jstatus;
	const char * dataType = env->GetStringUTFChars(jdatatype, NULL);
	const char * data = env->GetStringUTFChars(jdata, NULL);
	bool keepCallback = (jkeep == JNI_TRUE);

	LOGD("callback: %s, %s, %d, %s, %s", callbackId, success?"true":"false", statusCode, dataType, data);

	Magpie::instance()->callback( callbackId, success, statusCode, dataType, data, keepCallback );

	env->ReleaseStringUTFChars(jcallbackId, callbackId);
	env->ReleaseStringUTFChars(jdatatype, dataType);
	env->ReleaseStringUTFChars(jdata, data);
}

void Java_com_handywit_magpie_MagpieBridge_callbackJs(JNIEnv *env, jobject thiz, jstring jStatement)
{
	const char * js = env->GetStringUTFChars(jStatement, NULL);

	LOGD("callbackJs: %s", js);

	Magpie::instance()->callbackJs( js );

	env->ReleaseStringUTFChars(jStatement, js);
}


}
