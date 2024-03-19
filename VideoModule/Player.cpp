#pragma once
#include "pch.h"
#include "Player.h"

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

int Player::startReadThread()
{
    std::thread readThread(&Player::readThreadTask, this);
    readThread.detach();
    //readThread.join();
    return 0;
}


int Player::startDecodeThread()
{
    std::thread readThread(&Player::decodeThreadTask, this);
    readThread.detach();
    return 0;
}

int Player::startRenderThread()
{
    std::thread readThread(&Player::renderThreadTask, this);
    readThread.detach();
    return 0;
}


void Player::readThreadTask()
{
    //루프
        //컨텍스트로 부터 패킷 리드
        //리드된 패킷을 디코드 큐에 넣음.
    

    while (1)
    {
        AVPacket* packet = av_packet_alloc();
        if (av_read_frame(this->formatContext, packet) >= 0)
        {
            this->decodingQueue.push(packet);
        }
        else
        {
            break;
        }
    }
}

void Player::decodeThreadTask()
{
    //루프
        //디코드 큐에서 패킷 리드
        //디코더에 패킷 전달
            //루프
                //디코더에서 프레임 추출
                //랜더큐에 넣음

    int ret = 0;
    while (1)
    {
        if (this->decodingQueue.empty() == false)
        {
            AVPacket* packet = this->decodingQueue.front();
            this->decodingQueue.pop();

            ret = avcodec_send_packet(this->video_dec_ctx, packet);
            while (ret >= 0)
            {
                AVFrame * frame = av_frame_alloc();
                /*if (!frame) {
                    fprintf(stderr, "Could not allocate video frame\n");
                    return;
                }*/
                ret = avcodec_receive_frame(video_dec_ctx, frame);
                if (ret < 0)
                {
                    // those two return values are special and mean there is no output
                    // frame available, but there were no errors during decoding
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

                this->renderingQueue.push(frame);
            }
        }
    }
}

void Player::renderThreadTask()
{
    //루프
        //랜더큐에서 프레임 추출
        //프레임 랜더링

    //// RGB 데이터에 필요한 버퍼 크기 계산
    this->img_bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, this->width, this->height, 1);

    int ret = 0;
    while (1)
    {
        if (this->renderingQueue.empty() == false)
        {
            AVFrame* frame = this->renderingQueue.front();
            this->renderingQueue.pop();

            if (this->video_dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO)
            {
                AVFrame* afterConvertFrame;
                afterConvertFrame = av_frame_alloc();
                if (!afterConvertFrame) {
                    fprintf(stderr, "새 프레임 할당 실패\n");
                    continue;
                }
                //변환 프레임의 버퍼 할당
                if (av_image_alloc(afterConvertFrame->data, afterConvertFrame->linesize, this->width, this->height, AV_PIX_FMT_RGB24, 1) < 0) {
                    fprintf(stderr, "변환 프레임 버퍼 할당 실패\n");
                    continue;
                }
                this->swsCtx = sws_getContext(
                    this->width, this->height, this->pix_fmt,
                    this->width, this->height, AV_PIX_FMT_RGB24,
                    SWS_BILINEAR, NULL, NULL, NULL);
                if (!this->swsCtx) {
                    fprintf(stderr, "이미지 포멧 변환 불가\n");
                    continue;
                }

                // 변환 실행
                sws_scale(this->swsCtx,
                    (const uint8_t* const*)frame->data, frame->linesize,
                    0, frame->height,
                    afterConvertFrame->data, afterConvertFrame->linesize);

                //콜백 메서드 호출
                if (this->imageDecodedCallback != nullptr)
                {
                    this->imageDecodedCallback(afterConvertFrame->data[0], this->img_bufsize, this->width, this->height);
                    std::this_thread::sleep_for(std::chrono::milliseconds(33));
                }
            }
            else
            {

            }
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
}