cmake_minimum_required(VERSION 3.22.1)

project("wildpointerdemo")


set(FFMPEG ${CMAKE_SOURCE_DIR}/ffmpeg)
include_directories(${FFMPEG}/include)

file(GLOB cpp_src_file *.cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        ${cpp_src_file})

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        android

        ${FFMPEG}/libs/${ANDROID_ABI}/libavcodec.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libavdevice.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libavfilter.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libavutil.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libavformat.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libswresample.so
        ${FFMPEG}/libs/${ANDROID_ABI}/libswscale.so
        z # 引入z库
        EGL
        GLESv2
        android # 引入android库
        OpenSLES # 引入OpenSLES库
        log)