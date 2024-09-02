// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#pragma once
#include "pch.h"

#include <iostream>
#include <chrono>
#include <thread>
//#include "SDL.h"

#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2.lib")

//c 헤더를 c++에서 정상적으로 쓰기 위함 (랭글링 때문)
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

#include "Player.h"

#include "DirectX9Renderer.h"
#include "DirectX11Renderer.h"


#define INBUF_SIZE 4096

Player* player = nullptr;



// 콜백 함수 타입 정의
//typedef void (*ImgDecodedCallbackFunction)(uint8_t* buf, int size, int width, int height);
//ImgDecodedCallbackFunction imageDecodedCallback = nullptr;



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


extern "C" __declspec(dllexport) bool CreatePlayer()
{
    if (player == nullptr)
    {
        player = new Player();
        return true;
    }
    else
    {
        return false;
    }
}

extern "C" __declspec(dllexport) bool CreateDx11RenderScreenOnPlayer(HWND hWnd, int videoWidth, int videoHeight)
{
    if (player == nullptr)
    {
        return false;
    }
    else
    {
        return player->CreateVideoDx11RenderScreen(hWnd, videoWidth, videoHeight);
    }
}


// 콜백 함수 등록
extern "C" __declspec(dllexport) void RegisterOnImgDecodeCallback(OnImgDecodeCallbackFunction callback) 
{
    player->RegisterOnImageDecodeCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback)
{
    player->RegisterOnVideoLengthCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback)
{
    player->RegisterOnVideoProgressCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnBufferProgressCallback(OnBufferProgressCallbackFunction callback)
{
    player->RegisterOnBufferProgressCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnBufferStartPosCallback(OnBufferStartPosCallbackFunction callback)
{
    player->RegisterOnBufferStartPosCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnStartCallback(OnStartCallbackFunction callback)
{
    player->RegisterOnStartCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnPauseCallback(OnPauseCallbackFunction callback)
{
    player->RegisterOnPauseCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnResumeCallback(OnResumeCallbackFunction callback)
{
    player->RegisterOnResumeCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnStopCallback(OnStopCallbackFunction callback)
{
    player->RegisterOnStopCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnRenderTimingCallback(OnRenderTimingCallbackFunction callback)
{
    player->RegisterOnRenderTimingCallback(callback);
}

extern "C" __declspec(dllexport) void RegisterOnVideoSizeCallback(OnVideoSizeCallbackFunction callback)
{
    player->RegisterOnVideoSizeCallback(callback);
}

extern "C" __declspec(dllexport) void Cleanup()
{
    player->Cleanup();
}





extern "C" __declspec(dllexport) void RenderTestRectangleDx11() {
    if (player == nullptr) {
        return;
    }
    player->DrawDirectXTestRectangle();
}







extern "C" __declspec(dllexport) int Add(int a, int b) {
    unsigned version = avformat_version();
    printf("libavformat version: %u.%u.%u\n", version >> 16, (version >> 8) & 0xFF, version & 0xFF);
    return a + b;
}












extern "C" {
    __declspec(dllexport) DirectX9Renderer* CreateRenderer_DX9(HWND hwnd) {
        auto renderer = new DirectX9Renderer(hwnd);
        if (!renderer->Init()) {
            delete renderer;
            return nullptr;
        }
        return renderer;
    }

    __declspec(dllexport) void DestroyRenderer_DX9(DirectX9Renderer* renderer) {
        delete renderer;
    }

    __declspec(dllexport) void RenderRectangle_DX9(DirectX9Renderer* renderer) {
        if (renderer) {
            renderer->RenderRectangle();
        }
    }

    __declspec(dllexport) void RenderAVFrame_DX9(DirectX9Renderer* renderer, AVFrame* frame) {
        if (renderer) {
            renderer->RenderAVFrame(frame);
        }
    }

    __declspec(dllexport) IDirect3DSurface9* GetSurface_DX9(DirectX9Renderer* renderer) {
        if (renderer) {
            return renderer->GetSurface();
        }
        return nullptr;
    }
}















































































/// <summary>
/// 파일 스트림 열기
/// </summary>
extern "C" __declspec(dllexport) void OpenFileStream(const char* filePath, int* videoWidth, int* videoHeight)
{
    if (player == nullptr)
    {
        return;
    }

    player->openFileStream(filePath, videoWidth, videoHeight);

    return;
}





/// <summary>
/// 재생
/// </summary>
extern "C" __declspec(dllexport) void Play()
{
    player->play();

    return;
}





/// <summary>
/// 일시정지
/// </summary>
extern "C" __declspec(dllexport) void Pause()
{
    player->pause();

    return;
}





/// <summary>
/// 정지
/// </summary>
extern "C" __declspec(dllexport) void Stop()
{
    player->stop();

    return;
}





/// <summary>
/// 특정위치로 건너뛰기
/// </summary>
extern "C" __declspec(dllexport) void JumpPlayTime(double targetPercent)
{

    player->jumpPlayTime(targetPercent);

    return;
}





/// <summary>
/// Rtsp재생
/// </summary>
extern "C" __declspec(dllexport) void playRtsp(HWND hWnd)
{
    player->playRtsp(hWnd);

    return;
}




/// <summary>
/// Rtsp정지
/// </summary>
extern "C" __declspec(dllexport) void stopRtsp()
{
    player->stopRtsp();

    return;
}












































/// <summary>
/// RTSP 플레이어 만들기
/// </summary>
extern "C" __declspec(dllexport) bool initRtspPlayer(const char* rtspAddress)
{
    if (player->openRtspStream(rtspAddress) == false)
    {
        return false;
    }

    return true;
}



















































/// <summary>
/// 디먹스, 디코딩 예제(비디오만)
/// </summary>
extern "C" __declspec(dllexport) void RunDecodeExample1() 
{
    avformat_network_init();

    AVFormatContext* formatContext = nullptr;
    const AVCodec* videoDecoder = NULL;
    const AVCodec* audioDecoder = NULL;
    AVCodecContext* codecContext = NULL;
    AVCodecContext* video_dec_ctx = NULL, * audio_dec_ctx;
    AVPacket* packet;

    AVStream* videoStream;
    int width, height;
    enum AVPixelFormat pix_fmt;

    AVFrame* frame;
    AVFrame* afterConvertFrame;
    enum AVPixelFormat frame_pix_fmt;
    struct SwsContext* swsCtx;
    BYTE* img_buffer;
    int img_bufsize;


    int video_stream_idx = -1, audio_stream_idx = -1;




    const char* filename = "asdf.avi";


    int ret;
    int eof;



    if (avformat_open_input(&formatContext, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return;
    }

    ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), filename);
        return;
    }
    else
    {
        
        video_stream_idx = ret;
        videoStream = formatContext->streams[video_stream_idx];
        videoDecoder = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (!videoDecoder) {
            fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }
        video_dec_ctx = avcodec_alloc_context3(videoDecoder);
        if (!video_dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        if ((ret = avcodec_parameters_to_context(video_dec_ctx, videoStream->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        if ((ret = avcodec_open2(video_dec_ctx, videoDecoder, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
    }

    if (!videoStream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }

    afterConvertFrame = av_frame_alloc();
    if (!afterConvertFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }

    packet = av_packet_alloc();
    if (!packet)
    {
        return;
    }


    

    

    //패킷 리드
    while (av_read_frame(formatContext, packet) >= 0) 
    {
        //읽은 패킷이 비디오 스트림인 경우 처리
        if (packet->stream_index == video_stream_idx)
        {
            ret = avcodec_send_packet(video_dec_ctx, packet);
            if (ret < 0) {
                fprintf(stderr, "디코더에 패킷 전달 실패\n");
                break;
            }

            while (ret >= 0) 
            {
                ret = avcodec_receive_frame(video_dec_ctx, frame);
                if (ret < 0) 
                {
                    if (ret == AVERROR_EOF)
                    {
                        fprintf(stderr, "EOF\n");
                        ret = 0;
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN))
                    {
                        fprintf(stderr, "EAGAIN\n");
                        ret = 0;
                        break;
                    }

                    fprintf(stderr, "디코디드 프레임 가져오다가 실패\n");
                    break;
                }

                if (video_dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO) 
                {
                    fprintf(stderr, "비디오 패킷 디코디드\n");

                    width = frame->width;
                    height= frame->height;

                    //변환 전 이미지 포멧
                    frame_pix_fmt = (AVPixelFormat)frame->format;

                    afterConvertFrame = av_frame_alloc();
                    if (!afterConvertFrame) {
                        fprintf(stderr, "새 프레임 할당 실패\n");
                        continue;
                    }

                    //변환 프레임의 버퍼 할당
                    if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, width, height, AV_PIX_FMT_RGB24, 1) < 0) {
                        fprintf(stderr, "변환 프레임 버퍼 할당 실패\n");
                        continue;
                    }

                    // 이미지 포멧을 RGB로 변환 준비
                    swsCtx = sws_getContext(
                        width, height, frame_pix_fmt,
                        width, height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, NULL, NULL, NULL);
                    if (!swsCtx) {
                        fprintf(stderr, "이미지 포멧 변환 불가\n");
                        continue;
                    }

                    // 변환 실행
                    sws_scale(swsCtx,
                        (const uint8_t* const*)frame->data, frame->linesize,
                        0, frame->height,
                        afterConvertFrame->data, afterConvertFrame->linesize);

                    // 리소스 해제
                    sws_freeContext(swsCtx);

                    //// RGB 데이터에 필요한 버퍼 크기 계산
                    img_bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
                    
                }
                else
                {
                    fprintf(stderr, "패킷 디코디드\n");
                }


                av_frame_unref(frame);
                av_frame_unref(afterConvertFrame);
                if (ret < 0)
                    break;
            }
        }
        //읽은 프레임이 오디오 스트림인 경우 처리
        else if (packet->stream_index == audio_stream_idx)
        {
        }

        av_packet_unref(packet);
    }


end:


    avcodec_free_context(&video_dec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avformat_close_input(&formatContext);

    fprintf(stderr, "끝\n");

    return;
}


















































































extern "C" __declspec(dllexport) void RunDecodeAndSDLPlayExample() 
{
    avformat_network_init();

    AVFormatContext* formatContext = nullptr;
    const AVCodec* videoDecoder = NULL;
    const AVCodec* audioDecoder = NULL;
    AVCodecContext* codecContext = NULL;
    AVCodecContext* video_dec_ctx = NULL, * audio_dec_ctx;
    AVPacket* packet;

    AVStream* videoStream;
    int width, height;
    enum AVPixelFormat pix_fmt;

    AVFrame* frame;
    AVFrame* afterConvertFrame;
    enum AVPixelFormat frame_pix_fmt;
    struct SwsContext* swsCtx;
    BYTE* img_buffer;
    int img_bufsize;


    int video_stream_idx = -1, audio_stream_idx = -1;




    const char* filename = "asdf.avi";


    int ret;
    int eof;








    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Could not initialize SDL! (%s)\n", SDL_GetError());
        return;
    }

    //window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 270, SDL_WINDOW_OPENGL);
    window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Could not create window! (%s)\n", SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL) {
        printf("Could not create renderer! (%s)\n", SDL_GetError());
        return;
    }

    /*texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        1280, 720);*/
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        1280, 720);
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        return;
    }
















    if (avformat_open_input(&formatContext, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return;
    }

    ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), filename);
        return;
    }
    else
    {

        video_stream_idx = ret;
        videoStream = formatContext->streams[video_stream_idx];
        videoDecoder = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (!videoDecoder) {
            fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }
        video_dec_ctx = avcodec_alloc_context3(videoDecoder);
        if (!video_dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(video_dec_ctx, videoStream->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        /* Init the decoders */
        if ((ret = avcodec_open2(video_dec_ctx, videoDecoder, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return;
        }

        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
    }

    if (!videoStream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }

    afterConvertFrame = av_frame_alloc();
    if (!afterConvertFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }

    packet = av_packet_alloc();
    if (!packet)
    {
        return;
    }






    //패킷 리드
    while (av_read_frame(formatContext, packet) >= 0)
    {
        //읽은 패킷이 비디오 스트림인 경우 처리
        if (packet->stream_index == video_stream_idx)
        {
            ret = avcodec_send_packet(video_dec_ctx, packet);
            if (ret < 0) {
                fprintf(stderr, "디코더에 패킷 전달 실패\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(video_dec_ctx, frame);
                if (ret < 0) {
                    if (ret == AVERROR_EOF)
                    {
                        fprintf(stderr, "EOF\n");
                        ret = 0;
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN))
                    {
                        fprintf(stderr, "EAGAIN\n");
                        ret = 0;
                        break;
                    }

                    fprintf(stderr, "디코디드 프레임 가져오다가 실패\n");
                    break;
                }

                if (video_dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO)
                {
                    fprintf(stderr, "비디오 패킷 디코디드\n");

                    width = frame->width;
                    height = frame->height;

                    //변환 전 이미지 포멧
                    frame_pix_fmt = (AVPixelFormat)frame->format;

                    afterConvertFrame = av_frame_alloc();
                    if (!afterConvertFrame) {
                        fprintf(stderr, "새 프레임 할당 실패\n");
                        continue;
                    }

                    //변환 프레임의 버퍼 할당
                    if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, width, height, AV_PIX_FMT_RGB24, 1) < 0) {
                        fprintf(stderr, "변환 프레임 버퍼 할당 실패\n");
                        continue;
                    }

                    // 이미지 포멧을 RGB로 변환 준비
                    swsCtx = sws_getContext(
                        width, height, frame_pix_fmt,
                        width, height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, NULL, NULL, NULL);
                    if (!swsCtx) {
                        fprintf(stderr, "이미지 포멧 변환 불가\n");
                        continue;
                    }

                    // 변환 실행
                    sws_scale(swsCtx,
                        (const uint8_t* const*)frame->data, frame->linesize,
                        0, frame->height,
                        afterConvertFrame->data, afterConvertFrame->linesize);

                    // 리소스 해제
                    sws_freeContext(swsCtx);


                    SDL_UpdateTexture(texture, NULL,
                        afterConvertFrame->data[0], afterConvertFrame->linesize[0]);

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);






                    //// RGB 데이터에 필요한 버퍼 크기 계산
                    img_bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
                }
                else
                {
                    fprintf(stderr, "패킷 디코디드\n");
                }


                av_frame_unref(frame);
                av_frame_unref(afterConvertFrame);
                if (ret < 0)
                    break;
            }
        }
        //읽은 프레임이 오디오 스트림인 경우 처리
        else if (packet->stream_index == audio_stream_idx)
        {
        }

        av_packet_unref(packet);
    }


end:


    avcodec_free_context(&video_dec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avformat_close_input(&formatContext);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    fprintf(stderr, "끝\n");

    return;
}