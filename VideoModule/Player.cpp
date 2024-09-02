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

int Player::startDecodeAndRenderThread()
{
    this->decodeAndRenderThread = new std::thread(&Player::videoDecodeAndRenderThreadTask, this);

    if (this->progressCheckingThread == nullptr)
    {
        this->progressCheckingThread = new std::thread(&Player::progressCheckingThreadTask, this);
    }
    return 0;
}



int Player::startReadRtspThread()
{
    this->readRtspThread = new std::thread(&Player::readRtspThreadTask, this);
    return 0;
}

int Player::startDecodeAndRenderRtspThread()
{
    this->decodeAndRenderRtspThread = new std::thread(&Player::videoDecodeAndRenderRtspThreadTask, this);

    if (this->progressCheckingThread == nullptr)
    {
        this->progressCheckingThread = new std::thread(&Player::progressCheckingThreadTask, this);
    }
    return 0;
}

















































bool Player::openRtspStream(const char* rtspPath)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    if (rtspPath != nullptr)
    {
        this->inputSourcePath = rtspPath;
    }


    avformat_network_init();
    this->formatContext = avformat_alloc_context();
    if (avformat_open_input(&this->formatContext, this->inputSourcePath, NULL, NULL) < 0) {
        fprintf(stderr, "���� �ҽ� ���� ���� %s\n", this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return false;
    }

    printf("rtsp �����\n");
    return true;
}

void Player::readRtspThreadTask()
{
    if (avformat_find_stream_info(this->formatContext, NULL) < 0) {
        fprintf(stderr, "��Ʈ�� ���� ã�� ����\n");
        this->isStreamSourceOpen = false;
        return;
    }
    printf("rtsp ��Ʈ�� ���� ã��\n");

    int ret = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "%s ��Ʈ���� ã�� �� ���� '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }
    printf("rtsp ���� ��Ʈ�� ã��\n");

    this->video_stream_idx = ret;
    this->videoStream = this->formatContext->streams[video_stream_idx];
    this->videoDecoder = avcodec_find_decoder(this->videoStream->codecpar->codec_id);

    this->videoTimeBase = this->videoStream->time_base;
    this->videoTimeBase_ms = av_q2d(this->videoTimeBase) * 1000;//s���� -> ms������ ��ȯ
    if (this->videoStream->duration > 0)
    {
        this->duration_ms = this->videoStream->duration * av_q2d(this->videoTimeBase) * 1000;
    }
    if (this->videoStream->start_time == AV_NOPTS_VALUE)//��Ʈ���� ���� �ð��� �������� ���� ���
    {
        this->start_time = AV_NOPTS_VALUE;
    }
    else if (this->videoStream->start_time >= 0)
    {
        this->start_time = this->videoStream->start_time;//��Ʈ���� ���� �ð� ���(time_base ���� ��, PTS�� DTS�� ����)
    }
    this->fps = av_q2d(this->videoStream->avg_frame_rate);

    if (this->onVideoLengthCallback != nullptr)
    {
        this->onVideoLengthCallback((double)this->duration_ms);
    }

    double period = std::chrono::steady_clock::period::num / static_cast<double>(std::chrono::steady_clock::period::den);
    this->steadyClockTo_ms_coefficient = 1000.0 / period;

    if (!this->videoDecoder) {
        fprintf(stderr, "%s �ڵ� ã�� ����\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("�ڵ� ã��\n");
    this->video_dec_ctx = avcodec_alloc_context3(videoDecoder);
    if (!this->video_dec_ctx) {
        fprintf(stderr, "%s �ڵ� context �Ҵ� ����\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("�ڵ� context �Ҵ�\n");
    if ((ret = avcodec_parameters_to_context(this->video_dec_ctx, this->videoStream->codecpar)) < 0) {
        fprintf(stderr, "%s �ڵ� �Ķ���� ���� ����\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("�ڵ� �Ķ���� ����\n");
    if ((ret = avcodec_open2(this->video_dec_ctx, this->videoDecoder, NULL)) < 0) {
        fprintf(stderr, "%s �ڵ� ���� ����\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("�ڵ� ���� ����\n");

    this->width = this->video_dec_ctx->width;
    this->height = this->video_dec_ctx->height;
    this->pix_fmt = this->video_dec_ctx->pix_fmt;


    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);

    this->isStreamSourceOpen = true;



    if (this->onVideoSizeCallbackFunction != nullptr)
    {
        this->onVideoSizeCallbackFunction(this->width, this->height);
    }

    this->directx11Renderer = new DirectX11Renderer(this->hwnd_);
    if (this->directx11Renderer->Init(this->width, this->height) == false)
    {
        delete this->directx11Renderer;
    }





    //��Ŷ ó�� ���� ����
    std::vector<AVPacket*> tempBuffer;//������ GOP ������ ��Ŷ ������ �޴� ������ �ӽ� ����(I������ ~ ���� I������ ����)
    bool isFirstKeyFrameFound = false;
    this->playStarted = false;
    this->isReading = true;

    while (this->isReading == true) {

        if (this->isReading == false)
        {
            break;
        }

        AVPacket* packet = av_packet_alloc();
        
        if (av_read_frame(this->formatContext, packet) >= 0) //��Ʈ������ ��Ŷ �дµ� ������ ���
        {
            //���� ���� Ÿ�̹��� ���
            if (this->playStarted == false)
            {
                this->playStartedDts = packet->dts;//���� DTS ���
                this->playStarted = true;
            }
            else
            {
                this->lastestPacketDts = packet->dts;//���������� ���� ��Ŷ�� DTS ����
            }

            //ù Ű������ �߰� �� �Ϲ� ������
            if (isFirstKeyFrameFound == false &&
                packet->flags != AV_PKT_FLAG_KEY)
            {
                //Ű������ ���� �Ϲ� �������� �ǹ̰� �����Ƿ� ����
            }
            //ù Ű������ �߰�
            else if (isFirstKeyFrameFound == false &&
                packet->flags == AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
                isFirstKeyFrameFound = true;
            }
            //�� ��° Ű������ �߰� �� �Ϲ� ������
            else if (isFirstKeyFrameFound == true &&
                packet->flags != AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
            }
            //�� ��° Ű������ �߰�
            else if (isFirstKeyFrameFound == true &&
                packet->flags == AV_PKT_FLAG_KEY)
            {
                //GOP ����(ùI���� �ι�°I������)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //���ĵ� ��Ŷ���� ��¥ ��Ŷ ���ۿ� ����
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] {
                        return (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false);
                        });
                    if (this->isReading == false)
                    {
                        break;
                    }

                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("push packet DTS : %d    buffer size : %d\n", (int)packet->dts, (int)currentPacketBufferSize);

                        if (currentPacketBufferSize >= this->CAN_POP_PACKET_BUFFER_SIZE)
                        {
                            this->bufferCondVar.notify_all();
                        }
                    }
                }
                tempBuffer.clear();

                //�ι�° Ű�������� ù��° Ű���������� ���
                tempBuffer.push_back(packet);
            }

            {
                std::lock_guard<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                    this->bufferCondVar.notify_all();
                }
            }
        }
        else //���� �Ǵ� ���� ���
        {
            printf("��Ŷ �б� ����\n");
            //�ӽù��ۿ� GOP�� �����ִ� ��� ó��
            if (isFirstKeyFrameFound == true &&
                tempBuffer.empty() == false)
            {
                //GOP ����(ùI���� �ι�°I������)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //���ĵ� ��Ŷ���� ��¥ ��Ŷ ���ۿ� ����
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] { return this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false; });
                    if (this->isReading == false)
                    {
                        break;
                    }
                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("push packet DTS : %d    buffer size : %d\n", (int)packet->dts, (int)currentPacketBufferSize);
                    }
                }
                tempBuffer.clear();

                if (this->isReading == false)
                {
                    break;
                }

                {
                    std::lock_guard<std::mutex> lock(this->bufferMutex);
                    if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                        this->bufferCondVar.notify_all();
                    }
                }
            }
            isFirstKeyFrameFound = false;
        }
    }

    this->playStarted = false;
    this->isReading = false; 
    this->bufferCondVar.notify_all();
}

void Player::videoDecodeAndRenderRtspThreadTask()
{
    this->endOfDecoding = false;

    while (this->isReading) 
    {
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

        while (true)
        {
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
                printf(">>>>��Ŷ ���� ���� ��Ŷ:%d�� \n", this->packetBuffer.size());
                bufferCondVar.notify_all();
            }

            if (packet->stream_index != this->video_stream_idx)
            {
                continue;
            }

            int64_t lastPts = 0;
            AVFrame* frame = av_frame_alloc();
            if (avcodec_send_packet(this->video_dec_ctx, packet) >= 0)
            {
                

                printf("��Ŷ ���ڵ� �õ�\n");
                while (true)
                {
                    if (this->endOfDecoding == true)
                    {
                        return;
                    }

                    int ret = avcodec_receive_frame(this->video_dec_ctx, frame);
                    if (ret < 0)
                    {

                        if (ret == AVERROR_EOF)
                        {
                            fprintf(stderr, "               ���ڵ� ��� EOF\n");
                            ret = 0;
                            break;
                        }
                        else if (ret == AVERROR(EAGAIN))
                        {
                            fprintf(stderr, "               ���ڵ� ��� EAGAIN\n");
                            ret = 0;
                            break;
                        }

                        fprintf(stderr, "               ���ڵ�� ������ �������ٰ� ����\n");
                        break;
                    }
                    else
                    {
                        //////wpf image ������
                        //AVFrame* afterConvertFrame;
                        //afterConvertFrame = av_frame_alloc();
                        //if (!afterConvertFrame) {
                        //    continue;
                        //}

                        ////��ȯ �������� ���� �Ҵ�
                        //if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, /*AV_PIX_FMT_RGBA*/AV_PIX_FMT_RGB24, 1) < 0) {
                        //    continue;
                        //}

                        //// ��ȯ ����
                        //sws_scale(this->swsCtx,
                        //    (const uint8_t* const*)frame->data, frame->linesize,
                        //    0, frame->height,
                        //    afterConvertFrame->data, afterConvertFrame->linesize);

                        ////�ݹ� �޼��� ȣ��
                        //if (this->onImageDecodeCallback != nullptr)
                        //{
                        //    this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
                        //}
                        /////wpf image ������



                        //directX11 ������
                        this->directx11Renderer->Render(frame);

                        int64_t timeGap = 1 / this->fps * 1000;
                        printf("         pts : %d      time gap : %lldms\n", (int)frame->pts, timeGap);

                        //���� ������ ��� ����
                        std::this_thread::sleep_for(std::chrono::milliseconds(timeGap));
                        
                        lastPts = frame->pts;
                        //av_frame_unref(frame);
                    }
                }
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
    }
}


































































































void Player::openFileStream(const char* filePath, int* videoWidth, int* videoHeight)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    //filePath = "rtsp://localhost:8554/sex";
    if (filePath != nullptr)
    {
        this->inputSourcePath = filePath;
    }

    int ret = 0;

    avformat_network_init();

    if (avformat_open_input(&this->formatContext, this->inputSourcePath, NULL, NULL) < 0) {
        fprintf(stderr, "���� �ҽ� ���� ���� %s\n", this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }

    if (avformat_find_stream_info(this->formatContext, NULL) < 0) {
        fprintf(stderr, "��Ʈ�� ���� ã�� ���� \n");
        this->isStreamSourceOpen = false;
        return;
    }

    ret = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "%s ��Ʈ���� ã�� �� ���� '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }

    this->video_stream_idx = ret;
    this->videoStream = this->formatContext->streams[video_stream_idx];
    this->videoDecoder = avcodec_find_decoder(this->videoStream->codecpar->codec_id);

    this->videoTimeBase = this->videoStream->time_base;
    this->videoTimeBase_ms = av_q2d(this->videoTimeBase) * 1000;//s���� -> ms������ ��ȯ
    this->duration_ms = this->videoStream->duration * av_q2d(this->videoTimeBase) * 1000;
    if (this->videoStream->start_time == AV_NOPTS_VALUE)//��Ʈ���� ���� �ð��� �������� ���� ���
    {
        this->start_time = AV_NOPTS_VALUE;
    }
    else
    {
        this->start_time = this->videoStream->start_time;//��Ʈ���� ���� �ð� ���(time_base ���� ��, PTS�� DTS�� ����)
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
        this->isStreamSourceOpen = false;
        return;
    }
    this->video_dec_ctx = avcodec_alloc_context3(videoDecoder);
    if (!this->video_dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(this->video_dec_ctx, this->videoStream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }

    /* Init the decoders */
    if ((ret = avcodec_open2(this->video_dec_ctx, this->videoDecoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }

    this->width = this->video_dec_ctx->width;
    this->height = this->video_dec_ctx->height;
    this->pix_fmt = this->video_dec_ctx->pix_fmt;
    *videoWidth = this->width;
    *videoHeight = this->height;

    this->isStreamSourceOpen = true;

    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24 /*AV_PIX_FMT_RGB24*/, /*AV_PIX_FMT_RGBA*/
        SWS_BILINEAR, NULL, NULL, NULL);

    int result = av_seek_frame(this->formatContext, video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
}

bool playStarted = false;
int64_t playStartedDts = 0;
int64_t lastestPacketDts = 0;

/// <summary>
/// ��Ŷ�� �о ��ŶQ�� �����Ѵ�<br/>
/// �켱 �ӽù��ۿ� GOP ������ŭ�� �о���̰� GOP���� ��ŭ DTS ������ ������ �� ��Ŷ ���ۿ� �����Ѵ�.<br/>
/// </summary>
void Player::readThreadTask()
{
    std::vector<AVPacket*> tempBuffer;//������ GOP ������ ��Ŷ ������ �޴� ������ �ӽ� ����(I������ ~ ���� I������ ����)
    bool isFirstKeyFrameFound = false;
    this->playStarted = false;
    this->isReading = true;
    while (this->isReading == true) {
        //�����Ͻ������÷��� Ȯ��
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isWaitingAfterCommand = false;
            this->readingPauseCondVar.notify_all();
            //printf("    ���尡 ��Ƽ����\n");
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return (this->isReadingPaused == false || this->isReading == false); });
        }

        if (this->isReading == false)
        {
            break;
        }


        AVPacket* packet = av_packet_alloc();
        //printf("Ǫ��Ǫ��Ǫ��.................\n");
        //��Ʈ������ ��Ŷ �дµ� ������ ���
        if (av_read_frame(this->formatContext, packet) >= 0) {

            /*printf("read packet DTS : %d\n", (int)packet->dts);*/

            //���� ���� Ÿ�̹��� ���
            if (this->playStarted == false)
            {
                this->playStartedDts = packet->dts;//���� DTS ���
                this->playStarted = true;

                //���α׷����ٿ��� ��%�� �ش��ϴ� �ð����� ���
                double progressPercent = (double)packet->dts * this->videoTimeBase_ms / this->duration_ms * 100.0;
            }
            else
            {
                this->lastestPacketDts = packet->dts;//���������� ���� ��Ŷ�� DTS ����

                //���α׷����ٿ��� ��%�� �ش��ϴ� �ð����� ���
                double progressPercent = (double)packet->dts * this->videoTimeBase_ms / this->duration_ms * 100.0;

                if (this->onBufferProgressCallback != nullptr)
                {
                    //���� ���α׷����� ����
                    this->onBufferProgressCallback(progressPercent);
                }
            }

            //ù Ű������ �߰� �� �Ϲ� ������
            if (isFirstKeyFrameFound == false && 
                packet->flags != AV_PKT_FLAG_KEY)
            {
                //Ű������ ���� �Ϲ� �������� �ǹ̰� �����Ƿ� ����
            }
            //ù Ű������ �߰�
            else if (isFirstKeyFrameFound == false && 
                packet->flags == AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
                isFirstKeyFrameFound = true;
            }
            //�� ��° Ű������ �߰� �� �Ϲ� ������
            else if (isFirstKeyFrameFound == true &&
                packet->flags != AV_PKT_FLAG_KEY)
            {
                tempBuffer.push_back(packet);
            }
            //�� ��° Ű������ �߰�
            else if (isFirstKeyFrameFound == true && 
                packet->flags == AV_PKT_FLAG_KEY)
            {
                //GOP ����(ùI���� �ι�°I������)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //���ĵ� ��Ŷ���� ��¥ ��Ŷ ���ۿ� ����
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] {
                            return (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false);
                        });
                    if (this->isReading == false)
                    {
                        break;
                    }

                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("push packet DTS : %d    buffer size : %d\n", (int)packet->dts, (int)currentPacketBufferSize);

                        if (currentPacketBufferSize >= this->CAN_POP_PACKET_BUFFER_SIZE)
                        {
                            this->bufferCondVar.notify_all();
                        }
                    }
                }
                tempBuffer.clear();

                //�ι�° Ű�������� ù��° Ű���������� ���
                tempBuffer.push_back(packet);
            }

            {
                std::lock_guard<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                    this->bufferCondVar.notify_all();
                }
            }
            
        }
        //���� �Ǵ� ������ ���� ���
        else 
        {
            //�ӽù��ۿ� GOP�� �����ִ� ��� ó��
            if (isFirstKeyFrameFound == true &&
                tempBuffer.empty() == false)
            {
                //GOP ����(ùI���� �ι�°I������)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                //���ĵ� ��Ŷ���� ��¥ ��Ŷ ���ۿ� ����
                for (AVPacket* p : tempBuffer)
                {
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
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
                    }
                }
            }
            isFirstKeyFrameFound = false;

            {
                std::unique_lock<std::mutex> lock(this->bufferMutex);
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
                    //���� �˸��� ��¥ ��Ŷ
                    AVPacket* endPacket = av_packet_alloc();
                    endPacket->flags = PKT_END;
                    this->packetBuffer.push(endPacket);
                }
            }
            
            this->bufferCondVar.notify_all();
            continue;
        }
    }

    this->playStarted = false;
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
}



/// <summary>
/// ���ڵ� �� ������,
/// ���۰� CAN_POP_PACKET_BUFFER_SIZE�̻� ���� ���� ����,
/// �� �� �����ϸ� ���۰� �� �� ���� ��� ���ڵ�,
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

        
        //���ڵ��Ͻ������÷��� Ȯ��
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
            //���ڵ��Ͻ������÷��� Ȯ��
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
                bufferCondVar.notify_all();////
            }

            if (packet->stream_index != this->video_stream_idx)
            {
                continue;
            }

            //������ ��Ŷ�� ���� ���
            if (packet->flags == PKT_END)
            {
                //���α׷����� ����
                this->progress_percent = 100;
                av_packet_unref(packet);
                av_packet_free(&packet);
                fprintf(stderr, "   ������ ������ ������ ������ ��Ŷ\n");
                this->directx11Renderer->ClearScreen();

                this->endOfDecoding = true;

                auto t = new std::thread([this]() {
                    this->stop();
                    });

                fprintf(stderr, "   end\n");
                break;
            }


            AVFrame* frame = av_frame_alloc();
            //printf("               ������ ���ڵ� �Լ� ȣ�� DTS:%d\n", (int)packet->dts);
            if (avcodec_send_packet(video_dec_ctx, packet) >= 0)
            {
                while (true)
                {
                    //���ڵ��Ͻ������÷��� Ȯ��
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

                    
                    int ret = avcodec_receive_frame(video_dec_ctx, frame);
                    //printf("               ���ڵ��� ������ ó�� PTS:%d\n", (int)frame->pts);
                    if (ret < 0)
                    {
                        //av_frame_free(&frame);

                        if (ret == AVERROR_EOF)
                        {
                            fprintf(stderr, "               ���ڵ� ��� EOF\n");
                            ret = 0;
                            break;
                        }
                        else if (ret == AVERROR(EAGAIN))
                        {
                            fprintf(stderr, "               ���ڵ� ��� EAGAIN\n");
                            ret = 0;
                            break;
                        }

                        fprintf(stderr, "               ���ڵ�� ������ �������ٰ� ����\n");
                        break;
                    }
                    else
                    {                        
                        ////wpf image ������
                        AVFrame* afterConvertFrame;
                        afterConvertFrame = av_frame_alloc();
                        if (!afterConvertFrame) {
                            continue;
                        }

                        //��ȯ �������� ���� �Ҵ�
                        if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, /*AV_PIX_FMT_RGBA*/AV_PIX_FMT_RGB24, 1) < 0) {
                            continue;
                        }

                        //��ȯ ����
                        sws_scale(this->swsCtx,
                            (const uint8_t* const*)frame->data, frame->linesize,
                            0, frame->height,
                            afterConvertFrame->data, afterConvertFrame->linesize);

                        //�ݹ� �޼��� ȣ��
                        if (this->onImageDecodeCallback != nullptr)
                        {
                            this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
                        }
                        ////wpf image ������


                        //this->renderFrame();
                        /*if (this->onRenderTimingCallbackFunction != nullptr)
                        {
                            this->onRenderTimingCallbackFunction();
                        }*/



                        ////directX11 ������
                        this->directx11Renderer->Render(frame);




                        //���α׷����� ����
                        this->progress_percent = (double)frame->pts * videoTimeBase_ms / this->duration_ms * 100.0;
                        fprintf(stderr, "         pts : %d         ���α׷��� : %d \n", (int)frame->pts, this->progress_percent);

                        //���� ������ ��� ����
                        std::this_thread::sleep_for(std::chrono::milliseconds((long long)this->videoTimeBase_ms));
                        /*av_frame_unref(frame);
                        av_frame_free(&frame);*/
                    }

                }
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
            av_frame_unref(frame);
            av_frame_free(&frame);
        }

        bufferCondVar.notify_all();
    }

    //avcodec_free_context(&this->video_dec_ctx);
}




























































//void Player::videoRenderThreadTask()
//{
//    //����
//        //����ť���� ������ ����
//        //������ ������
//
//    //// RGB �����Ϳ� �ʿ��� ���� ũ�� ���
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
//            //�ð� ����
//            //���� �ð�
//            //���ο� �����̸� ( pts�� 0�̸�
//                //���� �ð��� starttime���� ���
//                //���� pts�� startPts���� ���
//            //���� ���̾�����
//                //���� �ð��� startime�� ���� ���(���� �� �帥 �ð�)
//            //pts�� ms ������ ��ȯ
//            int64_t timeSpent = 0;//���� �� ��� �ð�
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
//            //pts ��
//            //��ǥ �ð� ���� ���� ���
//                //��ٷȴٰ� ����
//            if (pts_ms - this->startPts_ms > timeSpent)
//            {
//                int64_t gap_ms = pts_ms - this->startPts_ms - timeSpent;
//                std::this_thread::sleep_for(std::chrono::milliseconds(gap_ms));
//                
//            }
//
//            //���� �ð� ���� ���� ���
//                //�� ���� �н��ؾ����� ���(��� �� : ���̳��� �ð� / Ÿ�� ���̽�)
//                //���� ���������� �н�
//            else if (pts_ms - this->startPts_ms < timeSpent)
//            {
//                int64_t gap_ms = timeSpent - (pts_ms - this->startPts_ms);
//                int numToPass = gap_ms / videoTimeBase_ms;
//                if (numToPass > 0)
//                {
//                    //���۸� �ʿ���
//                    printf("��������\n");
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
//                fprintf(stderr, "�� ������ �Ҵ� ����\n");
//                continue;
//            }
//            //��ȯ �������� ���� �Ҵ�
//            if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, AV_PIX_FMT_RGB24, 1) < 0) {
//                fprintf(stderr, "��ȯ ������ ���� �Ҵ� ����\n");
//                continue;
//            }
//            /*this->swsCtx = sws_getContext(
//                this->width, this->height, this->pix_fmt,
//                this->width, this->height, AV_PIX_FMT_RGB24,
//                SWS_BILINEAR, NULL, NULL, NULL);
//            if (!this->swsCtx) {
//                fprintf(stderr, "�̹��� ���� ��ȯ �Ұ�\n");
//                continue;
//            }*/
//
//
//            // ��ȯ ����
//            sws_scale(this->swsCtx,
//                (const uint8_t* const*)frame->data, frame->linesize,
//                0, frame->height,
//                afterConvertFrame->data, afterConvertFrame->linesize);
//
//            //�ݹ� �޼��� ȣ��
//            if (this->onImageDecodeCallback != nullptr)
//            {
//                this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
//            }
//        }
//    }
//}

















































































//0.5�� ���� ���α׷��� üũ
void Player::progressCheckingThreadTask()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (this->isPaused == false)
        {
            this->onVideoProgressCallback(this->progress_percent);
        }
        this->bufferCondVar.notify_all();
    }
}































int Player::playRtsp(HWND hWnd)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    this->hwnd_ = hWnd;
    this->startReadRtspThread();
    this->startDecodeAndRenderRtspThread();

    return 0;
}

int Player::stopRtsp()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    //���ڵ� ������ ����
    this->endOfDecoding = true;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
    if (this->decodeAndRenderRtspThread != nullptr &&
        this->decodeAndRenderRtspThread->joinable() == true)
    {
        this->decodeAndRenderRtspThread->join();
    }

    //���� ������ ����
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->readingPauseCondVar.notify_all();
    if (this->readRtspThread != nullptr &&
        this->readRtspThread->joinable() == true)
    {
        this->readRtspThread->join();
    }

    //��Ŷ���� clear
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

    //�÷��� ���� �ʱ�ȭ
    this->isDecodingPaused = false;
    this->isReadingPaused = false;
    this->endOfDecoding = false;
    this->isReading = true;

    //�ݹ� ȣ��
    if (this->onStopCallbackFunction != nullptr)
    {
        this->onStopCallbackFunction();
    }
    printf("stop return\n");

    return 0;
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

    //���ڵ� ������ ����
    this->endOfDecoding = true;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
    if (this->decodeAndRenderThread != nullptr && 
        this->decodeAndRenderThread->joinable() == true)
    {
        this->decodeAndRenderThread->join();
    }

    //���� ������ ����
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->readingPauseCondVar.notify_all();
    if (this->readThread != nullptr && 
        this->readThread->joinable() == true)
    {
        this->readThread->join();
    }

    //��Ŷ���� clear
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

    //�÷��� ���� �ʱ�ȭ
    this->isDecodingPaused = false;
    this->isReadingPaused = false;
    this->isPaused = false;
    this->isWaitingAfterCommand = false;
    this->endOfDecoding = false;
    this->isReading = true;

    //�ݹ� ȣ��
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
        //���徲���� �Ͻ�����
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isReadingPaused = true;
            this->readingPauseCondVar.notify_all();
            this->bufferCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //��Ŷ���� clear
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            while (!this->packetBuffer.empty())
            {
                this->packetBuffer.pop();
            }
        }

        //SEEK ���
        int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);

        //���� ������ �簳
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

    }
    else
    {
        if (this->decodeAndRenderThread->joinable() == false || this->readThread->joinable() == false)
        {
            return -1;
        }


        //���ڵ�,���� ������ �Ͻ� ����
        {
            std::unique_lock<std::mutex> decodingPauseMutexLock(this->decodingPauseMutex);
            this->isDecodingPaused = true;
            this->decodingPauseCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->decodingPauseCondVar.wait(decodingPauseMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //���� ������ �Ͻ� ����
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isReadingPaused = true;
            this->readingPauseCondVar.notify_all();
            this->bufferCondVar.notify_all();
            this->isWaitingAfterCommand = true;
            this->readingPauseCondVar.wait(readingMutexLock, [this] { return this->isWaitingAfterCommand == false; });
        }

        //��Ŷ���� clear
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            while (!this->packetBuffer.empty())
            {
                this->packetBuffer.pop();
            }
        }

        //SEEK ���
        int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);

        //���� ������ �簳
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

        //���ڵ�,���� ������ �簳
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
            //TODO : ���ۿ� ����
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

void Player::RegisterOnVideoSizeCallback(OnVideoSizeCallbackFunction callback)
{
    this->onVideoSizeCallbackFunction = callback;
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
    this->directx11Renderer->RenderEmptyRect();
}





























void Player::Cleanup() {
    if (video_dec_ctx) avcodec_free_context(&video_dec_ctx);
    if (formatContext) avformat_close_input(&formatContext);
    if (swsCtx) sws_freeContext(swsCtx);
}














Player::Player() {
    
}

Player::Player(HWND  hWnd) 
{
    this->hwnd_ = hWnd;
}