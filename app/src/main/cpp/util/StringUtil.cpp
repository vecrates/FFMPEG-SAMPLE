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


    template<typename ... Args>
    std::string string_format(const std::string &format, Args ... args) {
        size_t size = 1 + snprintf(nullptr, 0, format.c_str(), args ...);  // Extra space for \0
        // unique_ptr<char[]> buf(new char[size]);
        char bytes[size];
        snprintf(bytes, size, format.c_str(), args ...);
        return {bytes};
    }


}



