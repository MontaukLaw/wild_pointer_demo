#include "../include/BKCppPlayer.h"

BKCppPlayer::BKCppPlayer(const char *data_source, JNICallbakcHelper *helper) {

    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source); // 把源 Copy给成员

    this->helper = helper;

}

BKCppPlayer::~BKCppPlayer() {

    if (data_source) {
        delete data_source;
        data_source = nullptr;
    }

    if (helper) {
        delete helper;
        helper = nullptr;
    }

}

void BKCppPlayer::setRenderCallback(RenderCallback renderCallback_) {
    this->renderCallback = renderCallback_;
}

void BKCppPlayer::prepare_() { // 属于 子线程了 并且 拥有  DerryPlayer的实例 的 this

    formatContext = avformat_alloc_context();

    // 字典（键值对）
    AVDictionary *dictionary = nullptr;
    //设置超时（5秒）
    av_dict_set(&dictionary, "timeout", "5000000", 0); // 单位微妙

    /**
     * 1，AVFormatContext *
     * 2，路径 url:文件路径或直播地址
     * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
     * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
     */
    int r = avformat_open_input(&formatContext, data_source, nullptr, &dictionary);
    // 释放字典
    av_dict_free(&dictionary);
    if (r) {
        // 把错误信息反馈给Java，回调给Java  Toast【打开媒体格式失败，请检查代码】
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL, av_err2str(r));

            // char * errorInfo = av_err2str(r); // 根据你的返回值 得到错误详情
        }
        avformat_close_input(&formatContext);
        return;
    }

    r = avformat_find_stream_info(formatContext, nullptr);
    if (r < 0) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS, av_err2str(r));
        }
        avformat_close_input(&formatContext);
        return;
    }

    AVCodecContext *codecContext = nullptr;

    for (int stream_index = 0; stream_index < formatContext->nb_streams; ++stream_index) {
        AVStream *stream = formatContext->streams[stream_index];

        AVCodecParameters *parameters = stream->codecpar;

        AVCodec *codec = const_cast<AVCodec *>(avcodec_find_decoder(parameters->codec_id));
        if (!codec) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL, av_err2str(r));
            }
            avformat_close_input(&formatContext);
        }

        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL, av_err2str(r));
            }

            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);

            return;
        }

        r = avcodec_parameters_to_context(codecContext, parameters);
        if (r < 0) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, av_err2str(r));
            }
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);
            return;
        }

        r = avcodec_open2(codecContext, codec, nullptr);
        if (r) { // 非0就是true
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL, av_err2str(r));
            }
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);
            return;
        }

        AVRational time_base = stream->time_base;

        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {

            // 虽然是视频类型，但是只有一帧封面
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue;
            }

            AVRational fps_rational = stream->avg_frame_rate;
            int fps = av_q2d(fps_rational);

            // 是视频
            video_channel = new VideoChannel(stream_index, codecContext, time_base, fps);
            video_channel->setRenderCallback(renderCallback);
            video_channel->setJNICallbakcHelper(helper);
        }
    } // for end

    if (!video_channel) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_NOMEDIA, av_err2str(r));
        }
        if (codecContext) {
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        }
        avformat_close_input(&formatContext);
        return;
    }

    if (helper) { // 只要用户关闭了，就不准你回调给Java成 start播放
        helper->onPrepared(THREAD_CHILD);
    }
}

void *task_prepare(void *args) { // 此函数和DerryPlayer这个对象没有关系，你没法拿DerryPlayer的私有成员

    // avformat_open_input(0, this->data_source)

    auto *player = static_cast<BKCppPlayer *>(args);
    player->prepare_();
    return nullptr; // 必须返回，坑，错误很难找
}

void BKCppPlayer::prepare() {
    pthread_create(&pid_prepare, nullptr, task_prepare, this); // this == DerryPlayer的实例
}


void BKCppPlayer::stop_(BKCppPlayer *derryPlayer) {

    this->isPlaying = false;

    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);

    // pid_prepare pid_start 就全部停止下来了  稳稳的停下来
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }

    DELETE(video_channel);
    DELETE(derryPlayer);
}

void *task_stop(void *args) {
    auto *player = static_cast<BKCppPlayer *>(args);
    player->stop_(player);
    return nullptr;
}

void BKCppPlayer::stop() {

    // 只要用户关闭了，就不准你回调给Java成 start播放
    DELETE(helper)
    // helper = nullptr;
    if (video_channel) {
        video_channel->jniCallbakcHelper = nullptr;
    }

    // 如果是直接释放 我们的 prepare_ start_ 线程，不能暴力释放 ，否则会有bug

    // 让他 稳稳的停下来

    // 我们要等这两个线程 稳稳的停下来后，我再释放DerryPlayer的所以工作
    // 由于我们要等 所以会ANR异常

    // 所以我们我们在开启一个 stop_线程 来等你 稳稳的停下来
    // 创建子线程
    pthread_create(&pid_stop, nullptr, task_stop, this);

}



