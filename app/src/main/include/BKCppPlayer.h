#ifndef WILDPOINTERDEMO_BKCPPPLAYER_H
#define WILDPOINTERDEMO_BKCPPPLAYER_H

#include "JNICallbakcHelper.h"
#include "VideoChannel.h"

extern "C" { // ffmpeg是纯c写的，必须采用c的编译方式，否则奔溃
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class BKCppPlayer {
private :
    char *data_source = 0; // 指针 请赋初始值
    JNICallbakcHelper *helper = 0;
    RenderCallback renderCallback;
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0; // 媒体上下文 封装格式
    VideoChannel *video_channel = 0;
    pthread_t pid_stop;
    bool isPlaying; // 是否播放

public:

    BKCppPlayer(const char *data_source, JNICallbakcHelper *helper);

    ~BKCppPlayer();

    void setRenderCallback(RenderCallback renderCallback);

    void prepare();

    void prepare_();

    void stop();

    void stop_(BKCppPlayer *);

    void start();
};


#endif //WILDPOINTERDEMO_BKCPPPLAYER_H
