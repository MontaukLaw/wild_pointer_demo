#include <jni.h>
#include <string>
#include "../include/log4c.h"
#include "../include/BKCppPlayer.h"
#include "../include/util.h"
#include <jni.h>
#include <string>
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象

JavaVM *vm = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 锁

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_wildpointerdemo_BKJavaPlayer_setSurfaceNative(JNIEnv *env, jobject thiz, jobject surface, jlong native_obj) {
    auto *player = reinterpret_cast<BKCppPlayer *>(native_obj);
    if (!player) {
        LOGE("player is null");
        return;
    }
    pthread_mutex_lock(&mutex);

    // 先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }

    // 创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);

    pthread_mutex_unlock(&mutex);
}


// 函数指针的实现 实现渲染画面
void renderFrame(uint8_t *src_data, int width, int height, int src_linesize) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex); // 出现了问题后，必须考虑到，释放锁，怕出现死锁问题
    }

    // 设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_buffer;

    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;

        pthread_mutex_unlock(&mutex); // 解锁，怕出现死锁
        return;
    }

    // 填充[window_buffer]  画面就出来了  ==== 【目标 window_buffer】
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;

    for (int i = 0; i < window_buffer.height; ++i) { // 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // 通用的
        memcpy(dst_data + i * dst_linesize, src_data + i * src_linesize, dst_linesize); // OK的
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(window); // 解锁后 并且刷新 window_buffer的数据显示画面
    pthread_mutex_unlock(&mutex);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_wildpointerdemo_BKJavaPlayer_startNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    auto *player = reinterpret_cast<BKCppPlayer *>(native_obj);
    if (player) {
        player->start();
        LOGD("Start play");
    } else {
        LOGE("player is null");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_wildpointerdemo_BKJavaPlayer_stopNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    auto *player = reinterpret_cast<BKCppPlayer *>(native_obj);
    if (player) {
        player->stop();
        player->setRenderCallback(nullptr);
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_wulala_wildpointerdemo_BKJavaPlayer_prepareNative(JNIEnv *env, jobject thiz, jstring data_source) {
    const char *data_source_ = env->GetStringUTFChars(data_source, nullptr);
    auto *helper = new JNICallbakcHelper(vm, env, thiz); // C++子线程回调 ， C++主线程回调
    auto *player = new BKCppPlayer(data_source_, helper); // 有意为之的，开辟堆空间，不能释放
    player->setRenderCallback(renderFrame);
    // player->prepare();
    env->ReleaseStringUTFChars(data_source, data_source_);
    return reinterpret_cast<jlong>(player);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_wildpointerdemo_BKJavaPlayer_releaseNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    auto *player = reinterpret_cast<BKCppPlayer *>(native_obj);
    if (!player) {
        LOGE("player is null");
        return;
    }
    pthread_mutex_lock(&mutex);
    // 先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }

    pthread_mutex_unlock(&mutex);

    // 释放工作
    DELETE(player); // 在堆区开辟的 DerryPlayer.cpp 对象，已经被释放了哦
    DELETE(vm);
    DELETE(window);

}

void *task_start(void *args) {
    auto *player = static_cast<BKCppPlayer *>(args);
    if (player) {
        // player->start_();
    } else {
        LOGE("task_start player is null");
    }
    return nullptr; // 必须返回，坑，错误很难找
}

void BKCppPlayer::start() {
    LOGD("start");
    isPlaying = 1;

    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 视频播放
    if (video_channel) {
        // video_channel->start(); // 视频的播放
    } else {
        LOGE("video_channel is null");
    }
    LOGD("start task start thread");
    // 把 音频 视频 压缩包  加入队列里面去
    // 创建子线程 pthread
    pthread_create(&pid_start, nullptr, task_start, this); // this == DerryPlayer的实例
}


