//
// Created by weiyusong on 2023/2/8.
//

#include <jni.h>
#include <string>

namespace str_u {
    std::string jstring2string(JNIEnv *env, jstring jStr);

    template<typename ... Args>
    std::string string_format(const std::string &format, Args ... args);
}



