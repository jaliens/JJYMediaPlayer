#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <list>
#include <future>
#include <chrono>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <atomic>
#include "Monitor.h"
#include "FixedQueue.h"
#include "FixedMemQueue.h"
#include "pch.h"
#include "Monitor.h"
#include "DirectX11Renderer.h"

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

using namespace Microsoft::WRL;

typedef void (*OnImgDecodeCallbackFunction)(uint8_t* buf, int size, int width, int height);
typedef double (*OnVideoLengthCallbackFunction)(double length);
typedef double (*OnVideoProgressCallbackFunction)(double progress);
typedef double (*OnBufferProgressCallbackFunction)(double progress);
typedef double (*OnBufferStartPosCallbackFunction)(double progress);
typedef void (*OnStartCallbackFunction)();
typedef void (*OnPauseCallbackFunction)();
typedef void (*OnResumeCallbackFunction)();
typedef void (*OnStopCallbackFunction)();
typedef void (*OnRenderTimingCallbackFunction)();

class Player
{
public:
    Player();

    int decode_packet(AVCodecContext* dec, const AVPacket* pkt);

    void openFileStream(const char* filePath, int* videoWidth, int* videoHeight);
    int startReadThread();
    //int startRenderThread();
    int startDecodeAndRenderThread();
    int play();
    int pause();
    int resume();
    int stop();
    int jumpPlayTime(double seekPercent);

    void renderFrame();

    void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback);
    void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback);
    void RegisterOnBufferProgressCallback(OnVideoProgressCallbackFunction callback);
    void RegisterOnBufferStartPosCallback(OnBufferStartPosCallbackFunction callback);
    void RegisterOnImageDecodeCallback(OnImgDecodeCallbackFunction callback);
    void RegisterOnStartCallback(OnStartCallbackFunction callback);
    void RegisterOnPauseCallback(OnPauseCallbackFunction callback);
    void RegisterOnResumeCallback(OnResumeCallbackFunction callback);
    void RegisterOnStopCallback(OnResumeCallbackFunction callback);
    void RegisterOnRenderTimingCallback(OnRenderTimingCallbackFunction callback);
    
    void Cleanup();

    bool CreateVideoDx11RenderScreen(HWND hwnd, int videoWidth, int videoHeight);
    void DrawDirectXTestRectangle();

    DirectX11Renderer* directx11Renderer = nullptr;
private:

    AVFormatContext* formatContext = nullptr; // c++11 이상에선 NULL 보다 nullptr을 사용하면 타입 안전하다
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    const AVCodec* videoDecoder = nullptr;
    const AVCodec* audioDecoder = nullptr;
    AVStream* videoStream = nullptr;
    AVBufferPool* avBufferPool = nullptr;
    AVPacket* packet = nullptr;
    enum AVPixelFormat pix_fmt;
    AVRational videoTimeBase;//DTS나 PTS의 시간 단위를 나타냄
    double videoTimeBase_ms = 0;//DTS나 PTS의 시간 단위를 나타냄(예 : DTS가 1 증가분이 몇 ms 증가인지)
    double steadyClockTo_ms_coefficient = 0; // steadyClock의 단위를 ms로 바꾸기 위한 계수
    double fps = 0; //초당 프레임 레이트
    int64_t startPts_ms = 0;
    int64_t duration_ms = 0;
    int64_t start_time = 0;//스트림의 시작 시각(time_base 단위 즉, PTS와 DTS의 단위)
    int64_t progress_ms = 0;
    int64_t progress_percent = 0;
    //int64_t startTime_ms = 0;
    int64_t endTime_ms = 0;

    bool playStarted = false;//패킷 리드 시작됨 여부
    int64_t playStartedDts = 0;//패킷 리드 시작 시점의 dts
    int64_t lastestPacketDts = 0;//현재 리드된 패킷 dts


    int width, height;
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    struct SwsContext* swsCtx;
    int img_bufsize = 0;

    const char* inputFilename = "dddd.avi";
    bool isFileStreamOpen = false;

    //bool isRenderStarted = false;
    bool isDecodingPaused = false;
    bool isReadingPaused = false;
    bool isPaused = false;
    bool isWaitingAfterCommand = false;//명령 완료 대기 플래그
    bool endOfDecoding = false;
    bool isReading = true;

    /*std::queue<AVPacket*, std::list<AVPacket*>> decodingQueue;
    std::queue<AVFrame*, std::list<AVFrame*>> videoRenderingQueue;*/
    std::queue<AVPacket*> packetBuffer;//정렬된 패킷이 들어갈 버퍼


    FixedMemQueue* fixedMemRenderingQueue = nullptr;
    int linesize[AV_NUM_DATA_POINTERS] = {0};

    FixedQueue<AVPacket, 200> fixedDecodingQueue;
    FixedQueue<AVFrame, 200> fixedVideoRenderingQueue;

    std::thread* readThread = nullptr;
    std::thread* decodeThread = nullptr;
    std::thread* renderThread = nullptr;
    std::thread* decodeAndRenderThread = nullptr;
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
    OnBufferProgressCallbackFunction onBufferProgressCallback = nullptr;
    OnBufferStartPosCallbackFunction onBufferStartPosCallback = nullptr;
    OnStartCallbackFunction onStartCallbackFunction = nullptr;
    OnPauseCallbackFunction onPauseCallbackFunction = nullptr;
    OnResumeCallbackFunction onResumeCallbackFunction = nullptr;
    OnStopCallbackFunction onStopCallbackFunction = nullptr;
    OnRenderTimingCallbackFunction onRenderTimingCallbackFunction = nullptr;


    

    std::mutex bufferMutex;
    std::mutex readingPauseMutex;//리딩 쓰레드 정지 상태 확인 플래그용
    std::mutex decodingPauseMutex;//디코딩 쓰레드 정지 상태 확인 플래그용
    std::mutex waitingAfterCommandFlagMutex;//명령 완료 대기 확인 플래그용
    std::mutex commandMutex;//명령 완료 대기 확인 플래그용
    std::condition_variable bufferCondVar;
    std::condition_variable readingPauseCondVar;
    std::condition_variable decodingPauseCondVar;
    std::condition_variable waitingAfterCommandFlagCondVar;
    const int MAX_PACKET_BUFFER_SIZE = 60;//패킷버퍼 최대 사이즈
    const int CAN_POP_PACKET_BUFFER_SIZE = 30;//디코딩을 시작할 수 있는 최소 패킷 버퍼사이즈 


    static std::atomic<bool> decoding;
    static int g_width;
    static int g_height;


    void readThreadTask();
    /*void videoRenderThreadTask();*/
    void videoDecodeAndRenderThreadTask();
    void progressCheckingThreadTask();

};

