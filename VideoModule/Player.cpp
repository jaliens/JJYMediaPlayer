#pragma once

#include "pch.h"
#include "Monitor.h"
#include "Player.h"
#include <thread>
#include <future>
#include <chrono>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <atomic>
#include <iostream>

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


#define PKT_END     -0x9999


int Player::startReadThread()
{
    this->readThread = new std::thread(&Player::readThreadTask, this);
    return 0;
}


//int Player::startRenderThread()
//{
//    this->renderThread = new std::thread(&Player::videoRenderThreadTask, this);
//    this->renderThread->detach();
//
//    this->progressCheckingThread = new std::thread(&Player::progressCheckingThreadTask, this);
//    this->progressCheckingThread->detach();
//    return 0;
//}

int Player::startDecodeAndRenderThread()
{
    this->decodeAndRenderThread = new std::thread(&Player::videoDecodeAndRenderThreadTask, this);

    if (this->progressCheckingThread == nullptr)
    {
        this->progressCheckingThread = new std::thread(&Player::progressCheckingThreadTask, this);
    }
    return 0;
}































































bool playStarted = false;
int64_t playStartedDts = 0;
int64_t lastestPacketDts = 0;

/// <summary>
/// 패킷을 읽어서 패킷Q에 삽입한다<br/>
/// 우선 임시버퍼에 GOP 단위만큼만 읽어들이고 GOP단위 만큼 DTS 순으로 정렬한 후 패킷 버퍼에 삽입한다.<br/>
/// </summary>
void Player::readThreadTask()
{
    std::vector<AVPacket*> tempBuffer;//온전한 GOP 단위의 패킷 묶음을 받는 목적의 임시 버퍼(I프레임 ~ 다음 I프레임 직전)
    bool isFirstKeyFrameFound = false;
    playStarted = false;
    this->isReading = true;
    while (this->isReading == true) {
        //리딩일시정지플래그 확인
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isWaitingAfterCommand = false;
            this->readingPauseCondVar.notify_all();
            //printf("    리드가 노티파이\n");
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return (this->isReadingPaused == false || this->isReading == false); });
        }

        if (this->isReading == false)
        {
            break;
        }


        AVPacket* packet = av_packet_alloc();
        //printf("푸시푸시푸시.................\n");
        //스트림에서 패킷 읽는데 성공한 경우
        if (av_read_frame(this->formatContext, packet) >= 0) {

            //리드 시작 타이밍인 경우
            if (playStarted == false)
            {
                this->playStartedDts = packet->dts;//시작 DTS 기록
                this->playStarted = true;

                //프로그래스바에서 몇%에 해당하는 시간인지 계산
                double progressPercent = (double)packet->dts * videoTimeBase_ms / this->duration_ms * 100.0;
            }
            else
            {
                this->lastestPacketDts = packet->dts;//마지막으로 읽은 패킷의 DTS 갱신

                //프로그래스바에서 몇%에 해당하는 시간인지 계산
                double progressPercent = (double)packet->dts * videoTimeBase_ms / this->duration_ms * 100.0;

                if (this->onBufferProgressCallback != nullptr)
                {
                    //버퍼 프로그래스바 갱신
                    this->onBufferProgressCallback(progressPercent);
                }
            }

            //첫 키프레임 발견 전 일반 프레임
            if (isFirstKeyFrameFound == false && 
                packet->flags != AV_PKT_FLAG_KEY)
            {
                //키프레임 없는 일반 프레임은 의미가 없으므로 버림
            }
            //첫 키프레임 발견
            else if (isFirstKeyFrameFound == false && 
                packet->flags == AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
                isFirstKeyFrameFound = true;
            }
            //두 번째 키프레임 발견 전 일반 프레임
            else if (isFirstKeyFrameFound == true &&
                packet->flags != AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
            }
            //두 번째 키프레임 발견
            else if (isFirstKeyFrameFound == true && 
                packet->flags == AV_PKT_FLAG_KEY)
            {
                //GOP 정렬(첫I부터 두번째I전까지)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //정렬된 패킷들을 진짜 패킷 버퍼에 삽입
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(bufferMutex);
                    this->bufferCondVar.wait(lock, [this] {
                            return (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false);
                        });
                    if (this->isReading == false)
                    {
                        break;
                    }

                    if (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                    }
                }
                tempBuffer.clear();

                //두번째 키프레임을 첫번째 키프레임으로 취급
                tempBuffer.push_back(packet);
            }

            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                    this->bufferCondVar.notify_all();
                    //printf("디코딩 시작 가능 알림");
                }
            }
            
        }
        //에러 또는 파일이 끝난 경우
        else 
        {
            //임시버퍼에 GOP가 남아있는 경우 처리
            if (isFirstKeyFrameFound == true &&
                tempBuffer.empty() == false)
            {
                //GOP 정렬(첫I부터 두번째I전까지)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //정렬된 패킷들을 진짜 패킷 버퍼에 삽입
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(bufferMutex);
                    this->bufferCondVar.wait(lock, [this] { return this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false; });
                    if (this->isReading == false)
                    {
                        break;
                    }
                    if (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                    }
                }
                tempBuffer.clear();

                if (this->isReading == false)
                {
                    break;
                }

                {
                    std::lock_guard<std::mutex> lock(bufferMutex);
                    if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                        this->bufferCondVar.notify_all();
                        //printf("디코딩 시작 가능 알림(패킷 리드 끝)");
                    }
                }
            }
            isFirstKeyFrameFound = false;

            {
                std::unique_lock<std::mutex> lock(bufferMutex);
                this->bufferCondVar.wait(lock, [this] { return this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReading == false || this->isReadingPaused == true; });
                if (this->isReadingPaused == true)
                {
                    continue;
                }
                if (this->isReading == false)
                {
                    break;
                }
                if (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE)
                {
                    //끝을 알리는 가짜 패킷
                    AVPacket* endPacket = av_packet_alloc();
                    endPacket->flags = PKT_END;
                    this->packetBuffer.push(endPacket);
                }
            }
            

            //av_packet_free(&packet);
            this->bufferCondVar.notify_all();
            continue;
        }
    }

    playStarted = false;
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
    //avformat_close_input(&this->formatContext);
}



/// <summary>
/// 디코딩 후 랜더링,
/// 버퍼가 CAN_POP_PACKET_BUFFER_SIZE이상 차면 시작 가능,
/// 한 번 시작하면 버퍼가 빌 때 까지 계속 디코딩,
/// </summary>
void Player::videoDecodeAndRenderThreadTask()
{
    this->endOfDecoding = false;

    while (this->isReading && this->endOfDecoding == false) {
        {
            std::unique_lock<std::mutex> lock(this->bufferMutex);
            this->bufferCondVar.wait(lock, [this] { 
                return this->packetBuffer.empty() == false && 
                    (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE || this->isReading == false) ||
                    this->endOfDecoding == true;
                });
        }
        if (this->endOfDecoding == true)
        {
            return;
        }

        
        //디코딩일시정지플래그 확인
        {
            std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
            this->isWaitingAfterCommand = false;
            this->decodingPauseCondVar.notify_all();
            this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] { return this->isDecodingPaused == false || this->endOfDecoding == true; });
        }
        if (this->endOfDecoding == true)
        {
            return;
        }

        while (true) 
        {
            //디코딩일시정지플래그 확인
            {
                std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
                this->isWaitingAfterCommand = false;
                this->decodingPauseCondVar.notify_all();
                this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] { return this->isDecodingPaused == false || this->endOfDecoding == true; });
            }
            if (this->endOfDecoding == true)
            {
                return;
            }

            AVPacket* packet = nullptr;
            {
                std::unique_lock<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.empty() == true)
                {
                    break;
                }
                packet = this->packetBuffer.front();
                this->packetBuffer.pop();
            }

            if (packet->stream_index != this->video_stream_idx)
            {
                continue;
            }

            //마지막 패킷을 읽은 경우
            if (packet->flags == PKT_END)
            {
                //프로그래스바 갱신
                this->progress_percent = 100;
                av_packet_unref(packet);
                av_packet_free(&packet);
                fprintf(stderr, "   마지막 마지막 마지막 마지막 패킷\n");

                this->endOfDecoding = true;
                //종료0

                auto t = new std::thread([this]() {
                    this->stop();
                    });

                /*std::async(std::launch::async, [this]() {
                    this->stop();
                });*/
                fprintf(stderr, "   end\n");
                fprintf(stderr, "   end\n");
                break;
            }



            //printf("               프레임 디코드 함수 호출 DTS:%d\n", (int)packet->dts);
            if (avcodec_send_packet(video_dec_ctx, packet) >= 0)
            {
                while (true)
                {
                    //디코딩일시정지플래그 확인
                    {
                        std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
                        this->isWaitingAfterCommand = false;
                        this->decodingPauseCondVar.notify_all();
                        this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] { return this->isDecodingPaused == false || this->endOfDecoding == true; });
                    }
                    if (this->endOfDecoding == true)
                    {
                        return;
                    }

                    AVFrame* frame = av_frame_alloc();
                    int ret = avcodec_receive_frame(video_dec_ctx, frame);
                    //printf("               디코딩된 프레임 처리 PTS:%d\n", (int)frame->pts);
                    if (ret < 0)
                    {
                        av_frame_free(&frame);

                        if (ret == AVERROR_EOF)
                        {
                            fprintf(stderr, "               디코드 결과 EOF\n");
                            ret = 0;
                            break;
                        }
                        else if (ret == AVERROR(EAGAIN))
                        {
                            fprintf(stderr, "               디코드 결과 EAGAIN\n");
                            ret = 0;
                            break;
                        }

                        fprintf(stderr, "               디코드된 프레임 가져오다가 실패\n");
                        break;
                    }
                    else
                    {                        
                        //////wpf image 랜더링
                        //AVFrame* afterConvertFrame;
                        //afterConvertFrame = av_frame_alloc();
                        //if (!afterConvertFrame) {
                        //    continue;
                        //}

                        ////변환 프레임의 버퍼 할당
                        //if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, /*AV_PIX_FMT_RGBA*/AV_PIX_FMT_RGB24, 1) < 0) {
                        //    continue;
                        //}

                        //// 변환 실행
                        //sws_scale(this->swsCtx,
                        //    (const uint8_t* const*)frame->data, frame->linesize,
                        //    0, frame->height,
                        //    afterConvertFrame->data, afterConvertFrame->linesize);

                        ////콜백 메서드 호출
                        //if (this->onImageDecodeCallback != nullptr)
                        //{
                        //    this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
                        //}
                        /////wpf image 랜더링


                        //this->renderFrame();
                        /*if (this->onRenderTimingCallbackFunction != nullptr)
                        {
                            this->onRenderTimingCallbackFunction();
                        }*/



                        ////directX11 랜더링
                        this->directx11Renderer->Render(frame);
                        //this->directx11Renderer->Render();




                        //프로그래스바 갱신
                        this->progress_percent = (double)frame->pts * videoTimeBase_ms / this->duration_ms * 100.0;
                        fprintf(stderr, "               프로그래스 : %d \n", this->progress_percent);

                        //다음 프레임 재생 지연
                        std::this_thread::sleep_for(std::chrono::milliseconds((long long)this->videoTimeBase_ms));
                        av_frame_unref(frame);
                        av_frame_free(&frame);
                    }

                }
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
        }

        bufferCondVar.notify_all();
    }

    //avcodec_free_context(&this->video_dec_ctx);
}




























































//void Player::videoRenderThreadTask()
//{
//    //루프
//        //랜더큐에서 프레임 추출
//        //프레임 랜더링
//
//    //// RGB 데이터에 필요한 버퍼 크기 계산
//    this->img_bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, this->width, this->height, 1);
//
//    int ret = 0;
//
//    int popCount = 0;
//
//    while (1)
//    {
//        this->monitor_forRenderingThreadEnd.checkRequestAndWait();
//
//        {
//            std::lock_guard<std::mutex> pauseLock(this->pauseMtx);
//            bool isPaused = this->isPaused;
//        }
//
//        if (isPaused == true)
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)this->videoTimeBase_ms));
//            continue;
//        }
//        
//
//        AVFrame* frame = nullptr;
//        bool isRenderingQEmpty = false;
//
//        {
//            std::lock_guard<std::mutex> lock(this->renderingQmtx);
//            
//            isRenderingQEmpty = this->videoRenderingQueue.empty();
//            if (isRenderingQEmpty == false)
//            {
//                frame = this->videoRenderingQueue.front();
//                this->videoRenderingQueue.pop();
//            }
//        }
//
//        if (isRenderingQEmpty == true)
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//        else
//        {
//            
//            /*int a = frame->pict_type;
//            printf("type : %ld\n", a);*/
//
//            //시간 측정
//            //현재 시각
//            //새로운 시작이면 ( pts가 0이면
//                //현재 시각을 starttime으로 기록
//                //현재 pts를 startPts으로 기록
//            //진행 중이었으면
//                //현재 시각과 startime의 차이 계산(시작 후 흐른 시간)
//            //pts를 ms 단위로 변환
//            int64_t timeSpent = 0;//시작 후 경과 시간
//            auto now = std::chrono::steady_clock::now();
//            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
//            if (this->isRenderStarted == false)
//            {
//                this->startPts_ms = frame->pts * videoTimeBase_ms;
//                this->startTime_ms = now_ms;
//                this->isRenderStarted = true;
//            }
//            else
//            {
//                timeSpent = now_ms - this->startTime_ms;
//            }
//            int64_t pts_ms = frame->pts * videoTimeBase_ms;
//
//            //pts 비교
//            //목표 시간 보다 빠른 경우
//                //기다렸다가 진행
//            if (pts_ms - this->startPts_ms > timeSpent)
//            {
//                int64_t gap_ms = pts_ms - this->startPts_ms - timeSpent;
//                std::this_thread::sleep_for(std::chrono::milliseconds(gap_ms));
//                
//            }
//
//            //적정 시간 보다 느린 경우
//                //몇 개나 패스해야할지 계산(계산 법 : 차이나는 시간 / 타임 베이스)
//                //다음 프레임으로 패스
//            else if (pts_ms - this->startPts_ms < timeSpent)
//            {
//                int64_t gap_ms = timeSpent - (pts_ms - this->startPts_ms);
//                int numToPass = gap_ms / videoTimeBase_ms;
//                if (numToPass > 0)
//                {
//                    //버퍼링 필요함
//                    printf("으아으아\n");
//                    continue;
//                }
//            }
//            
//            if (this->onVideoProgressCallback != nullptr)
//            {
//                this->progress_ms = pts_ms;
//            }
//
//            AVFrame* afterConvertFrame;
//            afterConvertFrame = av_frame_alloc();
//            if (!afterConvertFrame) {
//                fprintf(stderr, "새 프레임 할당 실패\n");
//                continue;
//            }
//            //변환 프레임의 버퍼 할당
//            if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, AV_PIX_FMT_RGB24, 1) < 0) {
//                fprintf(stderr, "변환 프레임 버퍼 할당 실패\n");
//                continue;
//            }
//            /*this->swsCtx = sws_getContext(
//                this->width, this->height, this->pix_fmt,
//                this->width, this->height, AV_PIX_FMT_RGB24,
//                SWS_BILINEAR, NULL, NULL, NULL);
//            if (!this->swsCtx) {
//                fprintf(stderr, "이미지 포멧 변환 불가\n");
//                continue;
//            }*/
//
//
//            // 변환 실행
//            sws_scale(this->swsCtx,
//                (const uint8_t* const*)frame->data, frame->linesize,
//                0, frame->height,
//                afterConvertFrame->data, afterConvertFrame->linesize);
//
//            //콜백 메서드 호출
//            if (this->onImageDecodeCallback != nullptr)
//            {
//                this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
//            }
//        }
//    }
//}

















































































//0.5초 마다 프로그래스 체크
void Player::progressCheckingThreadTask()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (this->isPaused == false)
        {
            this->onVideoProgressCallback(this->progress_percent);
        }
    }
}



















































void Player::openFileStream(const char* filePath, int* videoWidth, int* videoHeight)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    if (filePath != nullptr)
    {
        this->inputFilename = filePath;
    }

    int ret = 0;

    avformat_network_init();

    if (avformat_open_input(&this->formatContext, this->inputFilename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", this->inputFilename);
        this->isFileStreamOpen = false;
        return;
    }

    if (avformat_find_stream_info(this->formatContext, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        this->isFileStreamOpen = false;
        return;
    }

    ret = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), this->inputFilename);
        this->isFileStreamOpen = false;
        return;
    }

    this->video_stream_idx = ret;
    this->videoStream = this->formatContext->streams[video_stream_idx];
    this->videoDecoder = avcodec_find_decoder(this->videoStream->codecpar->codec_id);

    this->videoTimeBase = this->videoStream->time_base;
    this->videoTimeBase_ms = av_q2d(this->videoTimeBase) * 1000;//s단위 -> ms단위로 변환
    this->duration_ms = this->videoStream->duration * av_q2d(this->videoTimeBase) * 1000;
    if (this->videoStream->start_time == AV_NOPTS_VALUE)//스트림의 시작 시각이 설정되지 않은 경우
    {
        this->start_time = AV_NOPTS_VALUE;
    }
    else
    {
        this->start_time = this->videoStream->start_time;//스트림의 시작 시각 기록(time_base 단위 즉, PTS와 DTS의 단위)
    }
    this->fps = av_q2d(this->videoStream->avg_frame_rate);

    if (this->onVideoLengthCallback != nullptr)
    {
        this->onVideoLengthCallback((double)this->duration_ms);
    }

    double period = std::chrono::steady_clock::period::num / static_cast<double>(std::chrono::steady_clock::period::den);
    this->steadyClockTo_ms_coefficient = 1000.0 / period;

    if (!this->videoDecoder) {
        fprintf(stderr, "Failed to find %s codec\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isFileStreamOpen = false;
        return;
    }
    this->video_dec_ctx = avcodec_alloc_context3(videoDecoder);
    if (!this->video_dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isFileStreamOpen = false;
        return;
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(this->video_dec_ctx, this->videoStream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isFileStreamOpen = false;
        return;
    }

    /* Init the decoders */
    if ((ret = avcodec_open2(this->video_dec_ctx, this->videoDecoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isFileStreamOpen = false;
        return;
    }

    this->width = this->video_dec_ctx->width;
    this->height = this->video_dec_ctx->height;
    this->pix_fmt = this->video_dec_ctx->pix_fmt;
    *videoWidth = this->width;
    *videoHeight = this->height;

    this->isFileStreamOpen = true;

    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24 /*AV_PIX_FMT_RGB24*/, /*AV_PIX_FMT_RGBA*/
        SWS_BILINEAR, NULL, NULL, NULL);

    int result = av_seek_frame(this->formatContext, video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
}

int Player::play()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    if (this->isPaused == true)
    {
        this->resume();
    }
    else
    {
        this->startReadThread();
        this->startDecodeAndRenderThread();
    }

    printf("play return\n");
    return 0;
}

int Player::pause()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    {
        std::unique_lock<std::mutex> decodingPauseMutexLock(this->decodingPauseMutex);
        this->isDecodingPaused = true;
        this->decodingPauseCondVar.notify_all();
        this->isWaitingAfterCommand = true;
        this->decodingPauseCondVar.wait(decodingPauseMutexLock, [this] { return this->isWaitingAfterCommand == false; });
    }

    this->isPaused = true;

    if (this->onPauseCallbackFunction != nullptr)
    {
        this->onPauseCallbackFunction();
    }

    printf("pause return\n");

    return 0;
}

int Player::resume()
{
    this->isDecodingPaused = false;
    this->decodingPauseCondVar.notify_all();

    this->isPaused = false;

    if (this->onResumeCallbackFunction != nullptr)
    {
        this->onResumeCallbackFunction();
    }

    printf("resume return\n");

    return 0;
}

int Player::stop()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    //디코딩 쓰레드 종료
    this->endOfDecoding = true;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
    if (this->decodeAndRenderThread != nullptr && 
        this->decodeAndRenderThread->joinable() == true)
    {
        this->decodeAndRenderThread->join();
    }

    //리딩 쓰레드 종료
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->readingPauseCondVar.notify_all();
    if (this->readThread != nullptr && 
        this->readThread->joinable() == true)
    {
        this->readThread->join();
    }

    //패킷버퍼 clear
    {
        std::unique_lock<std::mutex> lock(bufferMutex);
        while (!this->packetBuffer.empty())
        {
            this->packetBuffer.pop();
        }
    }

    if (this->formatContext != nullptr)
    {
        avformat_close_input(&this->formatContext);
    }

    //플래그 변수 초기화
    this->isDecodingPaused = false;
    this->isReadingPaused = false;
    this->isPaused = false;
    this->isWaitingAfterCommand = false;
    this->endOfDecoding = false;
    this->isReading = true;

    //콜백 호출
    if (this->onStopCallbackFunction != nullptr)
    {
        this->onStopCallbackFunction();
    }
    printf("stop return\n");

    return 0;
}


int Player::jumpPlayTime(double seekPercent)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    if (duration_ms == 0)
    {
        return -1;
    }

    double timebase_ms = av_q2d(this->videoTimeBase) * 1000;
    int64_t seekTime_ms = duration_ms * seekPercent / 100;
    int64_t seekTimeStamp = seekTime_ms / timebase_ms;

    if (this->isPaused == true)
    {
        //리드쓰레드 일시정지
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isReadingPaused = true;
            this->readingPauseCondVar.notify_all();
            this->bufferCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //패킷버퍼 clear
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            while (!this->packetBuffer.empty())
            {
                this->packetBuffer.pop();
            }
        }

        //SEEK 명령
        int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);

        //리드 쓰레드 재개
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

    }
    else
    {
        if (this->decodeAndRenderThread->joinable() == false || this->readThread->joinable() == false)
        {
            return -1;
        }


        //디코드,랜더 쓰레드 일시 정지
        {
            std::unique_lock<std::mutex> decodingPauseMutexLock(this->decodingPauseMutex);
            this->isDecodingPaused = true;
            this->decodingPauseCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->decodingPauseCondVar.wait(decodingPauseMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //리드 쓰레드 일시 정지
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isReadingPaused = true;
            this->readingPauseCondVar.notify_all();
            this->bufferCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //패킷버퍼 clear
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            while (!this->packetBuffer.empty())
            {
                this->packetBuffer.pop();
            }
        }

        //SEEK 명령
        int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);

        //리드 쓰레드 재개
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

        //디코드,랜더 쓰레드 재개
        this->isDecodingPaused = false;
        this->decodingPauseCondVar.notify_all();
        this->isPaused = false;
    }


    return 0;
}


int Player::decode_packet(AVCodecContext* decoderCtx, const AVPacket* pkt)
{
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(decoderCtx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding\n");
        return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        AVFrame* frame;
        frame = av_frame_alloc();

        ret = avcodec_receive_frame(decoderCtx, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            fprintf(stderr, "Error during decoding\n");
            return ret;
        }

        // write the frame data to output file
        if (decoderCtx->codec->type == AVMEDIA_TYPE_VIDEO)
        {
            //ret = output_video_frame(frame);
            //TODO : 버퍼에 삽입
        }
        else
        {
            //ret = output_audio_frame(frame);
        }

        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }

    return 0;
}

void Player::RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback)
{
    this->onVideoLengthCallback = callback;
}

void Player::RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback)
{
    this->onVideoProgressCallback = callback;
}

void Player::RegisterOnBufferProgressCallback(OnBufferProgressCallbackFunction callback)
{
    this->onBufferProgressCallback = callback;
}

void Player::RegisterOnBufferStartPosCallback(OnBufferStartPosCallbackFunction callback)
{
    this->onBufferStartPosCallback = callback;
}

void Player::RegisterOnImageDecodeCallback(OnImgDecodeCallbackFunction callback)
{
    this->onImageDecodeCallback = callback;
}

void Player::RegisterOnStartCallback(OnStartCallbackFunction callback)
{
    this->onStartCallbackFunction = callback;
}

void Player::RegisterOnPauseCallback(OnPauseCallbackFunction callback)
{
    this->onPauseCallbackFunction = callback;
}

void Player::RegisterOnResumeCallback(OnResumeCallbackFunction callback)
{
    this->onResumeCallbackFunction = callback;
}

void Player::RegisterOnStopCallback(OnStopCallbackFunction callback)
{
    this->onStopCallbackFunction = callback;
}

void Player::RegisterOnRenderTimingCallback(OnRenderTimingCallbackFunction callback)
{
    this->onRenderTimingCallbackFunction = callback;
}








































bool Player::CreateVideoDx11RenderScreen(HWND hwnd, int videoWidth, int videoHeight)
{
    this->directx11Renderer = new DirectX11Renderer(hwnd);
    if (this->directx11Renderer->Init(videoWidth, videoHeight) == false)
    {
        delete this->directx11Renderer;
        return false;
    }

}


void Player::DrawDirectXTestRectangle()
{
    if (this->directx11Renderer == nullptr)
    {
        return;
    }
    this->directx11Renderer->Render();
}





























void Player::Cleanup() {
    if (video_dec_ctx) avcodec_free_context(&video_dec_ctx);
    if (formatContext) avformat_close_input(&formatContext);
    if (swsCtx) sws_freeContext(swsCtx);
}














Player::Player() {
    
}