#pragma once

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

#include <mutex>
#include <iostream>
#include <future>
#include <chrono>
#include "pch.h"
#include "Player.h"
#include "Monitor.h"

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

int Player::startReadThread()
{
    this->readThread = new std::thread(&Player::readThreadTask, this);
    this->readThread->detach();
    return 0;
}


int Player::startDecodeThread()
{
    this->decodeThread = new std::thread(&Player::decodeThreadTask, this);
    this->decodeThread->detach();
    return 0;
}

int Player::startRenderThread()
{
    this->renderThread = new std::thread(&Player::videoRenderThreadTask, this);
    this->renderThread->detach();

    this->progressCheckingThread = new std::thread(&Player::progressCheckingThreadTask, this);
    this->progressCheckingThread->detach();
    return 0;
}


void Player::readThreadTask()
{
    //����
        //���ؽ�Ʈ�� ���� ��Ŷ ����
        //����� ��Ŷ�� ���ڵ� ť�� ����.
    

    while (1)
    {
        this->monitor_forReadingThreadEnd.checkRequestAndWait();

        AVPacket* packet = av_packet_alloc();
        int ret = av_read_frame(this->formatContext, packet);

        if (ret >= 0)
        {
            std::lock_guard<std::mutex> lock(this->decodingQmtx);
            this->decodingQueue.push(packet);


        }
        else
        {
        }
    }

}

void Player::decodeThreadTask()
{
    //����
        //���ڵ� ť���� ��Ŷ ����
        //���ڴ��� ��Ŷ ����
            //����
                //���ڴ����� ������ ����
                //����ť�� ����

    int ret = 0;
    while (1)
    {
        this->monitor_forDecodingThreadEnd.checkRequestAndWait();


        AVPacket* packet = nullptr;
        bool isDecodingQEmpty = false;

        AVPacket* testPacket = nullptr;//AVPacket �ʼ� �ʵ� �׽�Ʈ��
        testPacket = av_packet_alloc();

        {
            std::lock_guard<std::mutex> lock(this->decodingQmtx);
            isDecodingQEmpty = this->decodingQueue.empty();
            if (isDecodingQEmpty == false)
            {
                packet = this->decodingQueue.front();
                this->decodingQueue.pop();
            }
        }
       
        
        if (isDecodingQEmpty == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else if (isDecodingQEmpty == false)
        {
            if (packet->stream_index == this->video_stream_idx)
            {
                testPacket->data = packet->data;//AVPacket �ʼ� �ʵ� �׽�Ʈ��
                testPacket->size = packet->size;//AVPacket �ʼ� �ʵ� �׽�Ʈ��
                testPacket->stream_index = packet->stream_index;//AVPacket �ʼ� �ʵ� �׽�Ʈ��

                ret = avcodec_send_packet(this->video_dec_ctx, packet);
                while (ret >= 0)
                {
                    AVFrame* frame = av_frame_alloc();
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
                            ret = 0;
                            break;
                        }

                        fprintf(stderr, "���ڵ�� ������ �������ٰ� ����\n");
                        break;
                    }

                    //����
                    {
                        std::lock_guard<std::mutex> lock(this->renderingQmtx);
                        this->videoRenderingQueue.push(frame);
                    }
                }
            }
            else if (packet->stream_index == this->audio_stream_idx)
            {

            }
        }
    }
}













































void Player::videoRenderThreadTask()
{
    //����
        //����ť���� ������ ����
        //������ ������

    //// RGB �����Ϳ� �ʿ��� ���� ũ�� ���
    this->img_bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, this->width, this->height, 1);

    int ret = 0;

    int popCount = 0;

    while (1)
    {
        this->monitor_forRenderingThreadEnd.checkRequestAndWait();

        {
            std::lock_guard<std::mutex> pauseLock(this->pauseMtx);
            bool isPaused = this->isPaused;
        }

        if (isPaused == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)this->videoTimeBase_ms));
            continue;
        }
        

        AVFrame* frame = nullptr;
        bool isRenderingQEmpty = false;

        {
            std::lock_guard<std::mutex> lock(this->renderingQmtx);
            
            isRenderingQEmpty = this->videoRenderingQueue.empty();
            if (isRenderingQEmpty == false)
            {
                frame = this->videoRenderingQueue.front();
                this->videoRenderingQueue.pop();
            }
        }

        if (isRenderingQEmpty == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
            
            /*int a = frame->pict_type;
            printf("type : %ld\n", a);*/

            //�ð� ����
            //���� �ð�
            //���ο� �����̸� ( pts�� 0�̸�
                //���� �ð��� starttime���� ���
                //���� pts�� startPts���� ���
            //���� ���̾�����
                //���� �ð��� startime�� ���� ���(���� �� �帥 �ð�)
            //pts�� ms ������ ��ȯ
            int64_t timeSpent = 0;//���� �� ��� �ð�
            auto now = std::chrono::steady_clock::now();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            if (this->isRenderStarted == false)
            {
                this->startPts_ms = frame->pts * videoTimeBase_ms;
                this->startTime_ms = now_ms;
                this->isRenderStarted = true;
            }
            else
            {
                timeSpent = now_ms - this->startTime_ms;
            }
            int64_t pts_ms = frame->pts * videoTimeBase_ms;

            //pts ��
            //��ǥ �ð� ���� ���� ���
                //��ٷȴٰ� ����
            if (pts_ms - this->startPts_ms > timeSpent)
            {
                int64_t gap_ms = pts_ms - this->startPts_ms - timeSpent;
                std::this_thread::sleep_for(std::chrono::milliseconds(gap_ms));
                
            }

            //���� �ð� ���� ���� ���
                //�� ���� �н��ؾ����� ���(��� �� : ���̳��� �ð� / Ÿ�� ���̽�)
                //���� ���������� �н�
            else if (pts_ms - this->startPts_ms < timeSpent)
            {
                int64_t gap_ms = timeSpent - (pts_ms - this->startPts_ms);
                int numToPass = gap_ms / videoTimeBase_ms;
                if (numToPass > 0)
                {
                    //���۸� �ʿ���
                    printf("��������\n");
                    continue;
                }
            }
            
            if (this->onVideoProgressCallback != nullptr)
            {
                this->progress_ms = pts_ms;
            }

            AVFrame* afterConvertFrame;
            afterConvertFrame = av_frame_alloc();
            if (!afterConvertFrame) {
                fprintf(stderr, "�� ������ �Ҵ� ����\n");
                continue;
            }
            //��ȯ �������� ���� �Ҵ�
            if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, AV_PIX_FMT_RGB24, 1) < 0) {
                fprintf(stderr, "��ȯ ������ ���� �Ҵ� ����\n");
                continue;
            }
            /*this->swsCtx = sws_getContext(
                this->width, this->height, this->pix_fmt,
                this->width, this->height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, NULL, NULL, NULL);
            if (!this->swsCtx) {
                fprintf(stderr, "�̹��� ���� ��ȯ �Ұ�\n");
                continue;
            }*/


            // ��ȯ ����
            sws_scale(this->swsCtx,
                (const uint8_t* const*)frame->data, frame->linesize,
                0, frame->height,
                afterConvertFrame->data, afterConvertFrame->linesize);

            //�ݹ� �޼��� ȣ��
            if (this->onImageDecodeCallback != nullptr)
            {
                this->onImageDecodeCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
            }
        }
    }
}




////����Ÿ ���� �׽�Ʈ��
//void Player::decodeThreadTask()
//{
//    AVFrame* frame = av_frame_alloc();
//
//    int ret = 0;
//    while (1)
//    {
//        this->monitor_forDecodingThreadEnd.checkRequestAndWait();
//
//        AVPacket* packet = nullptr;
//        bool isDecodingQEmpty = false;
//        AVPacket copiedPacket;
//
//        {
//            std::lock_guard<std::mutex> lock(this->decodingQmtx);
//            isDecodingQEmpty = this->decodingQueue.empty();
//            if (isDecodingQEmpty == false)
//            {
//                packet = this->decodingQueue.front();
//                this->decodingQueue.pop();
//            }
//        }
//
//
//        if (isDecodingQEmpty == true)
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//        else if (isDecodingQEmpty == false)
//        {
//            if (packet->stream_index == this->video_stream_idx)
//            {
//                ret = avcodec_send_packet(this->video_dec_ctx, packet);
//                while (ret >= 0)
//                {
//                    
//                    ret = avcodec_receive_frame(video_dec_ctx, frame);
//                    if (ret < 0)
//                    {
//                        if (ret == AVERROR_EOF)
//                        {
//                            fprintf(stderr, "EOF\n");
//                            ret = 0;
//                            av_frame_unref(frame);
//                            break;
//                        }
//                        else if (ret == AVERROR(EAGAIN))
//                        {
//                            ret = 0;
//                            av_frame_unref(frame);
//                            break;
//                        }
//
//                        fprintf(stderr, "���ڵ�� ������ �������ٰ� ����\n");
//                        av_frame_unref(frame);
//                        break;
//                    }
//
//                    AVFrame* pushFrame = nullptr;
//                    AVFrame* afterConvertFrame = nullptr;
//
//                    //���Ǹ� ���� RGB24�� ��ȯ
//                    if (this->pix_fmt == AVPixelFormat::AV_PIX_FMT_RGB24)
//                    {
//                        pushFrame = frame;
//                    }
//                    else
//                    {
//                        afterConvertFrame = av_frame_alloc();
//
//                        if (!afterConvertFrame) {
//                            fprintf(stderr, "�� ������ �Ҵ� ����\n");
//                            continue;
//                        }
//
//                        if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, AV_PIX_FMT_RGB24, 1) < 0) {
//                            fprintf(stderr, "��ȯ ������ ���� �Ҵ� ����\n");
//                            continue;
//                        }
//
//                        sws_scale(this->swsCtx,
//                            (const uint8_t* const*)frame->data, frame->linesize,
//                            0, frame->height,
//                            afterConvertFrame->data, afterConvertFrame->linesize);
//
//                        pushFrame = afterConvertFrame;
//                    }
//
//
//                    {
//                        std::lock_guard<std::mutex> lock(this->renderingQmtx);
//                        int frameHeight = frame->height;
//                        int frameWidth = frame->width;
//                        pushFrame->linesize;
//
//                        //Ǫ�� ������ ���(RGB�� ��ȯ�ϸ� ������ data[0]�� ����)
//                        int frameSize = 0;
//                        for (int i = 0; i < AV_NUM_DATA_POINTERS; i++)
//                        {
//                            this->linesize[i] = pushFrame->linesize[i];
//                            if (pushFrame->linesize[i] > 0)
//                            {
//                                int planeSize = pushFrame->linesize[i] * frameHeight;
//                                frameSize += planeSize;
//                            }
//                        }
//                        int ptsSize = sizeof(int64_t);
//                        int frameAndPtsSize = frameSize + ptsSize;//pts�� Ÿ���� int64_t�̹Ƿ�
//
//                        //Ǫ�ð� �Ұ��� ��� ���
//                        while (this->fixedMemRenderingQueue->canPush(frameAndPtsSize) == false)
//                        {
//                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                        }
//
//                        //Ǫ��(RGB�� ��ȯ�ϸ� ������ data[0]�� ����)
//                        for (int i = 0; i < AV_NUM_DATA_POINTERS; i++)
//                        {
//                            //pts ����Ÿ Ǫ��
//                            this->fixedMemRenderingQueue->pushPts(pushFrame->pts);
//
//                            //data Ǫ��
//                            if (pushFrame->linesize[i] > 0)
//                            {
//                                int planeSize = pushFrame->linesize[i] * frameHeight;
//                                this->fixedMemRenderingQueue->push(pushFrame->data[i], planeSize);
//                            }
//                        }
//                    }
//                    av_frame_unref(frame);
//                    if (afterConvertFrame != nullptr)
//                    {
//                        av_frame_unref(afterConvertFrame);
//                    }
//                }
//            }
//            else if (packet->stream_index == this->audio_stream_idx)
//            {
//
//            }
//        }
//    }
//}





////���� �׽�Ʈ��
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (this->isPaused == false)
        {
            this->onVideoProgressCallback(this->progress_ms);
        }
    }
}

void Player::openFileStream() 
{
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
    this->videoTimeBase_ms = av_q2d(this->videoTimeBase) * 1000;//s���� -> ms������ ��ȯ
    this->duration_ms = this->videoStream->duration * av_q2d(this->videoTimeBase) * 1000;

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

    this->isFileStreamOpen = true;

    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);
}

int Player::play()
{
    if (this->isPaused == true)
    {
        this->isPaused = false;
        return 0;
    }

    this->startReadThread();
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    this->startDecodeThread();
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    this->startRenderThread();

    return 0;
}

int Player::pause()
{
    std::lock_guard<std::mutex> pauseLock(this->pauseMtx);
    this->isPaused = true;
    return 0;
}

int Player::resume()
{
    std::lock_guard<std::mutex> pauseLock(this->pauseMtx);
    this->isPaused = false;
    return 0;
}

int Player::stop()
{

    return 0;
}

int Player::JumpPlayTime(double seekPercent)
{
    int64_t seekTime_ms = duration_ms * seekPercent / 100;
    int64_t seekTime_s = seekTime_ms / 1000.0;

    double timebase = av_q2d(this->videoTimeBase);
    int64_t valuePerOneSec = 1 / timebase;
    int64_t seekTimeStamp = valuePerOneSec* seekTime_s;
    
    this->monitor_forReadingThreadEnd.requestHoldAndWait();
    this->monitor_forDecodingThreadEnd.requestHoldAndWait();
    this->monitor_forRenderingThreadEnd.requestHoldAndWait();

    //ť ����
    {
        std::lock_guard<std::mutex> decodingQLock(this->decodingQmtx);
        std::lock_guard<std::mutex> renderingQLock(this->renderingQmtx);

        while (this->decodingQueue.empty() == false)
        {
            this->decodingQueue.pop();
        }

        while (this->videoRenderingQueue.empty() == false)
        {
            this->videoRenderingQueue.pop();
        }
    }

    printf("ť ���� decQ:%ld   rndrQ:%ld \n", (long)this->decodingQueue.size(), (long)this->videoRenderingQueue.size());

    int64_t timestamp = 1 * AV_TIME_BASE;
    //���� ��Ʈ�� �󿡼� �����ġ �̵�
    printf("seek : %ld", seekTime_ms);
    int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);

    this->isRenderStarted = false;

    this->monitor_forReadingThreadEnd.notify();
    this->monitor_forDecodingThreadEnd.notify();
    this->monitor_forRenderingThreadEnd.notify();
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
