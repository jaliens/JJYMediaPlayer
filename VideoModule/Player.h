#pragma once

#include <mutex>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <list>
#include "Monitor.h"

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


typedef void (*OnImgDecodeCallbackFunction)(uint8_t* buf, int size, int width, int height);
typedef double (*OnVideoLengthCallbackFunction)(double length);
typedef double (*OnVideoProgressCallbackFunction)(double progress);
typedef void (*OnStartCallbackFunction)();
typedef void (*OnPauseCallbackFunction)();
typedef void (*OnResumeCallbackFunction)();

class Player
{
public:
    int decode_packet(AVCodecContext* dec, const AVPacket* pkt);
    void openFileStream();
    int startReadThread();
    int startDecodeThread();
    int startRenderThread();
    int play();
    int pause();
    int resume();
    int stop();
    int JumpPlayTime(double seekPercent);

    void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback);
    void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback);
    void RegisterOnImageDecodeCallback(OnImgDecodeCallbackFunction callback);
    void RegisterOnStartCallback(OnStartCallbackFunction callback);
    void RegisterOnPauseCallback(OnPauseCallbackFunction callback);
    void RegisterOnResumeCallback(OnResumeCallbackFunction callback);
    
private:
    AVFormatContext* formatContext = nullptr; // c++11 이상에선 NULL 보다 nullptr을 사용하면 타입 안전하다
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    const AVCodec* videoDecoder = nullptr;
    const AVCodec* audioDecoder = nullptr;
    AVStream* videoStream = nullptr;
    AVPacket* packet = nullptr;
    enum AVPixelFormat pix_fmt;
    AVRational videoTimeBase;
    double videoTimeBase_ms = 0;
    double steadyClockTo_ms_coefficient = 0; // steadyClock의 단위를 ms로 바꾸기 위한 계수
    int64_t startPts_ms = 0;
    int64_t duration_ms = 0;
    int64_t progress_ms = 0;
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

    bool isPaused = false;

    std::queue<AVPacket*, std::list<AVPacket*>> decodingQueue;
    std::queue<AVFrame*, std::list<AVFrame*>> videoRenderingQueue;

    std::thread* readThread = nullptr;
    std::thread* decodeThread = nullptr;
    std::thread* renderThread = nullptr;
    std::thread* progressCheckingThread = nullptr;

    std::mutex renderingQmtx;
    std::mutex decodingQmtx;
    std::mutex pauseMtx;

    Monitor monitor_forReadingThreadEnd;
    Monitor monitor_forDecodingThreadEnd;
    Monitor monitor_forRenderingThreadEnd;

    OnImgDecodeCallbackFunction onImageDecodeCallback = nullptr;
    OnVideoLengthCallbackFunction onVideoLengthCallback = nullptr;
    OnVideoProgressCallbackFunction onVideoProgressCallback = nullptr;
    OnStartCallbackFunction onStartCallbackFunction = nullptr;
    OnPauseCallbackFunction onPauseCallbackFunction = nullptr;
    OnResumeCallbackFunction onResumeCallbackFunction = nullptr;


    void readThreadTask();
    void decodeThreadTask();
    void videoRenderThreadTask();
    void progressCheckingThreadTask();
};

