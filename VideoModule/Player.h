#pragma once

#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <list>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>

#include <SDL.h>
}



class Player
{
public:
    int decode_packet(AVCodecContext* dec, const AVPacket* pkt);
    int startReadThread();
    int startDecodeThread();
    int startRenderThread();
    void readThreadTask();
    void decodeThreadTask();
    void videoRenderThreadTask();
    void openFileStream();

    typedef void (*ImgDecodedCallbackFunction)(uint8_t* buf, int size, int width, int height);
    ImgDecodedCallbackFunction imageDecodedCallback = nullptr;
private:
    AVFormatContext* formatContext = nullptr; // c++11 �̻󿡼� NULL ���� nullptr�� ����ϸ� Ÿ�� �����ϴ�
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    const AVCodec* videoDecoder = nullptr;
    const AVCodec* audioDecoder = nullptr;
    AVStream* videoStream = nullptr;
    AVPacket* packet = nullptr;
    enum AVPixelFormat pix_fmt;
    AVRational videoTimeBase;
    double videoTimeBase_ms = 0;
    double steadyClockTo_ms_coefficient = 0; // steadyClock�� ������ ms�� �ٲٱ� ���� ���
    int64_t duration_ms = 0;
    int64_t startTime_ms = 0;
    int64_t endTime_ms = 0;
    bool isRenderStarted = false;

    int width, height;
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    struct SwsContext* swsCtx;
    int img_bufsize = 0;

    const char* inputFilename = "dddd.avi";
    bool isFileStreamOpen = false;


    std::queue<AVPacket*, std::list<AVPacket*>> decodingQueue;
    std::queue<AVFrame*, std::list<AVFrame*>> videoRenderingQueue;
};

