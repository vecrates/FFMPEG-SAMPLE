cmake_minimum_required(VERSION 3.4.1)

set(CPP_DIR ${CMAKE_SOURCE_DIR}/src/main/cpp)
set(SO_LIB ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})

#FFMPEG-4.4
include_directories(${CPP_DIR}/include)
add_library(
        avcodec
        SHARED
        IMPORTED
)
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libavcodec.so)
add_library(
        avfilter
        SHARED
        IMPORTED
)
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libavfilter.so)
add_library(
        avformat
        SHARED
        IMPORTED
)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libavformat.so)
add_library(
        avutil
        SHARED
        IMPORTED
)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libavutil.so)
add_library(
        swresample
        SHARED
        IMPORTED
)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libswresample.so)
add_library(
        swscale
        SHARED
        IMPORTED
)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libswscale.so)

file(GLOB_RECURSE FF_PLAYER_SRC
        ${CPP_DIR}/*.cpp
        ${CPP_DIR}/*.c
)
add_library(
        ffplayer
        SHARED
        ${FF_PLAYER_SRC}
)
target_link_libraries(
        ffplayer

        log
        android

        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
)