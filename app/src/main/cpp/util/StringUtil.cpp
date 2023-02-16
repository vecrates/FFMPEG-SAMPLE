//
// Created by weiyusong on 2023/2/8.
//

#include "StringUtil.h"

namespace str_u {

    std::string jstring2string(JNIEnv *env, jstring jStr) {
        if (!jStr) {
            return "";
        }

        const jclass stringClass = env->GetObjectClass(jStr);
        const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes",
                                                    "(Ljava/lang/String;)[B");
        const auto stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes,
                                                                     env->NewStringUTF("UTF-8"));

        auto length = (size_t) env->GetArrayLength(stringJbytes);
        jbyte *pBytes = env->GetByteArrayElements(stringJbytes, nullptr);

        std::string ret = std::string((char *) pBytes, length);
        env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

        env->DeleteLocalRef(stringJbytes);
        env->DeleteLocalRef(stringClass);
        return ret;
    }

}



