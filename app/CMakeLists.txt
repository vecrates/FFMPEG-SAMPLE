cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CPP_DIR ${CMAKE_SOURCE_DIR}/src/main/cpp)
set(SO_LIB ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})

#FFMPEG-4.4
include_directories(${CPP_DIR}/ffmpeg/include)
include_directories(${CPP_DIR}/ffmpeg/)
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
add_library(
        x264
        SHARED
        IMPORTED
)
set_target_properties(x264
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libx264.so)
add_library(
        postproc
        SHARED
        IMPORTED
)
set_target_properties(postproc
        PROPERTIES IMPORTED_LOCATION
        ${SO_LIB}/libpostproc.so)

file(GLOB_RECURSE FF_PLAYER_SRC
        ${CPP_DIR}/*.cpp
        ${CPP_DIR}/*.c
        ${CPP_DIR}/*.h
        )
add_library(
        ffmpeg
        SHARED
        ${FF_PLAYER_SRC}
)
target_link_libraries(
        ffmpeg

        log
        android
        #        EGL
        #        GLESv2

        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
        x264
        postproc
)