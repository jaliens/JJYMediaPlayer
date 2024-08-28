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

    AVFormatContext* formatContext = nullptr; // c++11 �̻󿡼� NULL ���� nullptr�� ����ϸ� Ÿ�� �����ϴ�
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    const AVCodec* videoDecoder = nullptr;
    const AVCodec* audioDecoder = nullptr;
    AVStream* videoStream = nullptr;
    AVBufferPool* avBufferPool = nullptr;
    AVPacket* packet = nullptr;
    enum AVPixelFormat pix_fmt;
    AVRational videoTimeBase;//DTS�� PTS�� �ð� ������ ��Ÿ��
    double videoTimeBase_ms = 0;//DTS�� PTS�� �ð� ������ ��Ÿ��(�� : DTS�� 1 �������� �� ms ��������)
    double steadyClockTo_ms_coefficient = 0; // steadyClock�� ������ ms�� �ٲٱ� ���� ���
    double fps = 0; //�ʴ� ������ ����Ʈ
    int64_t startPts_ms = 0;
    int64_t duration_ms = 0;
    int64_t start_time = 0;//��Ʈ���� ���� �ð�(time_base ���� ��, PTS�� DTS�� ����)
    int64_t progress_ms = 0;
    int64_t progress_percent = 0;
    //int64_t startTime_ms = 0;
    int64_t endTime_ms = 0;

    bool playStarted = false;//��Ŷ ���� ���۵� ����
    int64_t playStartedDts = 0;//��Ŷ ���� ���� ������ dts
    int64_t lastestPacketDts = 0;//���� ����� ��Ŷ dts


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
    bool isWaitingAfterCommand = false;//��� �Ϸ� ��� �÷���
    bool endOfDecoding = false;
    bool isReading = true;

    /*std::queue<AVPacket*, std::list<AVPacket*>> decodingQueue;
    std::queue<AVFrame*, std::list<AVFrame*>> videoRenderingQueue;*/
    std::queue<AVPacket*> packetBuffer;//���ĵ� ��Ŷ�� �� ����


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
    std::mutex readingPauseMutex;//���� ������ ���� ���� Ȯ�� �÷��׿�
    std::mutex decodingPauseMutex;//���ڵ� ������ ���� ���� Ȯ�� �÷��׿�
    std::mutex waitingAfterCommandFlagMutex;//��� �Ϸ� ��� Ȯ�� �÷��׿�
    std::mutex commandMutex;//��� �Ϸ� ��� Ȯ�� �÷��׿�
    std::condition_variable bufferCondVar;
    std::condition_variable readingPauseCondVar;
    std::condition_variable decodingPauseCondVar;
    std::condition_variable waitingAfterCommandFlagCondVar;
    const int MAX_PACKET_BUFFER_SIZE = 60;//��Ŷ���� �ִ� ������
    const int CAN_POP_PACKET_BUFFER_SIZE = 30;//���ڵ��� ������ �� �ִ� �ּ� ��Ŷ ���ۻ����� 


    static std::atomic<bool> decoding;
    static int g_width;
    static int g_height;


    void readThreadTask();
    /*void videoRenderThreadTask();*/
    void videoDecodeAndRenderThreadTask();
    void progressCheckingThreadTask();

};

