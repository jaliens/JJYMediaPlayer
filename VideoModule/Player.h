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
    void renderThreadTask();
    void openFileStream();
private:
    AVFormatContext* formatContext = nullptr; // c++11 이상에선 NULL 보다 nullptr을 사용하면 타입 안전하다
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    const AVCodec* videoDecoder = nullptr;
    const AVCodec* audioDecoder = nullptr;
    AVStream* videoStream = nullptr;
    AVPacket* packet = nullptr;
    enum AVPixelFormat pix_fmt;
    int width, height;
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    const char* inputFilename = "asdf.avi";
    bool isFileStreamOpen = false;

    std::queue<AVPacket*, std::list<AVPacket*>> decodingQueue;
    std::queue<AVFrame*, std::list<AVFrame*>> renderingQueue;
};

