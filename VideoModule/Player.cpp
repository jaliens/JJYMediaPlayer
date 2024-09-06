#pragma once

#include "pch.h"
#include "Player.h"



#define PKT_END     -0x9999

using namespace std::chrono;

void Player::RegisterWindowHandle(HWND hWnd)
{
    this->hwnd_ = hWnd;
}

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

/// <summary>
/// 패킷 버퍼 비우기
/// </summary>
void Player::clearPacketBuffer()
{
    while (true)
    {
        AVPacket* packet = nullptr;
        std::unique_lock<std::mutex> lock(this->bufferMutex);
        if (this->packetBuffer.empty() == true)
        {
            break;
        }
        packet = this->packetBuffer.front();
        this->packetBuffer.pop();
        av_packet_unref(packet);
        av_packet_free(&packet);
        bufferCondVar.notify_all();
        this->bufferRrogress_percent = this->packetBuffer.size() / (double)this->MAX_RTSP_PACKET_BUFFER_SIZE * 100;
    }
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
        fprintf(stderr, "영상 소스 열기 실패 %s\n", this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return false;
    }

    printf("rtsp 연결됨\n");
    return true;
}

void Player::readRtspThreadTask()
{
    if (avformat_find_stream_info(this->formatContext, NULL) < 0) {
        fprintf(stderr, "스트림 정보 찾기 실패\n");
        this->isStreamSourceOpen = false;
        return;
    }
    printf("rtsp 스트림 정보 찾음\n");

    int ret = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "%s 스트림을 찾을 수 없음 '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }
    printf("rtsp 비디오 스트림 찾음\n");

    this->video_stream_idx = ret;
    this->videoStream = this->formatContext->streams[video_stream_idx];

    //CUDA 지원이 되는 포멧이면 CUDA 지원 코덱 가져오기
    if (this->IsCudaSupportedCodec(this->videoStream->codecpar->codec_id) == true)
    {
        this->videoDecoder = this->GetCudaCodecById(this->videoStream->codecpar->codec_id);
    }
    else
    {
        this->videoDecoder = avcodec_find_decoder(this->videoStream->codecpar->codec_id);
    }

    this->videoTimeBase = this->videoStream->time_base;
    this->videoTimeBase_ms = av_q2d(this->videoTimeBase) * 1000;//s단위 -> ms단위로 변환
    if (this->videoStream->duration > 0)
    {
        this->duration_ms = this->videoStream->duration * av_q2d(this->videoTimeBase) * 1000;
    }
    if (this->videoStream->start_time == AV_NOPTS_VALUE)//스트림의 시작 시각이 설정되지 않은 경우
    {
        this->start_time = AV_NOPTS_VALUE;
    }
    else if (this->videoStream->start_time >= 0)
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
        fprintf(stderr, "%s 코덱 찾기 실패\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("코덱 찾음\n");
    this->video_dec_ctx = avcodec_alloc_context3(videoDecoder);
    if (!this->video_dec_ctx) {
        fprintf(stderr, "%s 코덱 context 할당 실패\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("코덱 context 할당\n");
    if ((ret = avcodec_parameters_to_context(this->video_dec_ctx, this->videoStream->codecpar)) < 0) {
        fprintf(stderr, "%s 코덱 파라미터 설정 실패\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("코덱 파라미터 설정\n");
    if ((ret = avcodec_open2(this->video_dec_ctx, this->videoDecoder, NULL)) < 0) {
        fprintf(stderr, "%s 코덱 열기 실패\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        this->isStreamSourceOpen = false;
        return;
    }
    printf("코덱 열기 성공\n");

    this->width = this->video_dec_ctx->width;
    this->height = this->video_dec_ctx->height;
    this->pix_fmt = this->video_dec_ctx->pix_fmt;

    this->isStreamSourceOpen = true;

    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);




    if (this->onVideoSizeCallbackFunction != nullptr)
    {
        this->onVideoSizeCallbackFunction(this->width, this->height);
    }

    this->directx11Renderer = new DirectX11Renderer(this->hwnd_);
    if (this->directx11Renderer->Init(this->width, this->height) == false)
    {
        delete this->directx11Renderer;
    }





    //패킷 처리 로직 시작
    std::vector<AVPacket*> tempBuffer;//온전한 GOP 단위의 패킷 묶음을 받는 목적의 임시 버퍼(I프레임 ~ 다음 I프레임 직전)
    bool isFirstKeyFrameFound = false;
    this->playStarted = false;
    this->isReading = true;
    this->dtsIncrement = 0;
    this->playStartedDts = 0;
    this->lastestPacketDts = 0;
    bool isFirstDtsSet = false;
    int64_t firstDts = -1;

    while (this->isReading == true) 
    {
        if (this->isReading == false)
        {
            break;
        }

        AVPacket* packet = av_packet_alloc();
        
        if (av_read_frame(this->formatContext, packet) >= 0) //스트림에서 패킷 읽는데 성공한 경우
        {
            //dts정보가 비정상이면 버리기
            if (packet->dts < 0)
            {
                av_packet_unref(packet);
                av_packet_free(&packet);
                continue;
            }

            //리드 시작 타이밍인 경우
            if (this->playStarted == false)
            {
                this->playStartedDts = packet->dts;//시작 DTS 기록
                this->playStarted = true;
            }
            else
            {
                this->lastestPacketDts = packet->dts;//마지막으로 읽은 패킷의 DTS 갱신
            }

            printf("av_read_frame dts : %lld \n", packet->dts);

            //첫 키프레임 발견 전 일반 프레임
            if (isFirstKeyFrameFound == false &&
                packet->flags != AV_PKT_FLAG_KEY)
            {
                //키프레임 없는 일반 프레임은 의미가 없으므로 버림
                av_packet_unref(packet);
                av_packet_free(&packet);
            }
            //첫 키프레임 발견
            else if (isFirstKeyFrameFound == false &&
                packet->flags == AV_PKT_FLAG_KEY)
            {
                printf("첫 번째 키 프레임 발견 \n");
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
                printf("두 번째 키 프레임 발견 \n");

                //GOP 정렬(첫I부터 두번째I전까지)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                int64_t prevDts = -1;
                int64_t prevCurrentGap = 0;
                //정렬된 패킷들을 진짜 패킷 버퍼에 삽입
                for (AVPacket* p : tempBuffer)
                {
                    //DTS 최소 간격 계산 : PTS가 없을 때 임시 PTS 계산을 위함
                    if (prevDts >= 0)
                    {
                        prevCurrentGap = p->dts - prevDts;
                        if (prevCurrentGap > 0 &&
                            prevCurrentGap <= this->dtsIncrement ||
                            this->dtsIncrement == 0)
                        {
                            this->dtsIncrement = prevCurrentGap;
                        }
                    }
                    prevDts = p->dts;

                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] {
                        return (this->packetBuffer.size() < this->MAX_RTSP_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false);
                        });
                    if (this->isReading == false)
                    {
                        av_packet_unref(p);
                        av_packet_free(&p);
                        continue;
                    }

                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_RTSP_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("push packet DTS : %lld    buffer size : %zu\n", p->dts, currentPacketBufferSize);
                        this->bufferRrogress_percent = this->packetBuffer.size() / (double)this->MAX_RTSP_PACKET_BUFFER_SIZE * 100;
                        if (currentPacketBufferSize >= this->CAN_POP_RTSP_PACKET_BUFFER_SIZE)
                        {
                            this->bufferCondVar.notify_all();
                        }
                    }
                }
                tempBuffer.clear();

                //두번째 키프레임을 첫번째 키프레임으로 취급
                tempBuffer.push_back(packet);
            }

            {
                std::lock_guard<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.size() >= this->CAN_POP_RTSP_PACKET_BUFFER_SIZE) {
                    this->bufferCondVar.notify_all();
                }
            }
        }
        else //에러 또는 끝난 경우
        {
            printf("패킷 읽기 에러\n");
            av_packet_unref(packet);
            av_packet_free(&packet);

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
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] { return this->packetBuffer.size() < this->MAX_RTSP_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false; });
                    if (this->isReading == false)
                    {
                        av_packet_unref(p);
                        av_packet_free(&p);
                        continue;
                    }
                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_RTSP_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("remains_ push packet DTS : %lld    buffer size : %zu\n", p->dts, currentPacketBufferSize);
                        this->bufferRrogress_percent = this->packetBuffer.size() / (double)this->MAX_RTSP_PACKET_BUFFER_SIZE * 100;
                        if (this->packetBuffer.size() >= this->CAN_POP_RTSP_PACKET_BUFFER_SIZE) {
                            this->bufferCondVar.notify_all();
                        }
                    }
                }

                tempBuffer.clear();

                if (this->isReading == false)
                {
                    break;
                }

                {
                    std::lock_guard<std::mutex> lock(this->bufferMutex);
                    if (this->packetBuffer.size() >= this->CAN_POP_RTSP_PACKET_BUFFER_SIZE) {
                        this->bufferCondVar.notify_all();
                    }
                }
            }
            isFirstKeyFrameFound = false;

            break;
        }

    }

    for (AVPacket* p : tempBuffer)
    {
        av_packet_unref(p);
        av_packet_free(&p);
    }
    tempBuffer.clear();

    this->playStarted = false;
    this->isReading = false; 
    this->bufferCondVar.notify_all();

    /*avcodec_free_context(&this->video_dec_ctx);*/
    avformat_close_input(&this->formatContext);
    avformat_network_deinit();
    sws_freeContext(this->swsCtx);
}

void Player::videoDecodeAndRenderRtspThreadTask()
{
    bool isFirstFrameRendered = false;//첫 프레임이 랜더링 되었냐 여부
    int64_t firstFrameRenderTime_ms;//첫 프레임이 랜더링된 시각
    int64_t firstFramePts;//첫 재생 프레임의 pts
    int64_t nextFrameRenderTime_ms;//다음 프레임 랜더링 예정 시각
    int64_t fakePts = -1;//frame에 PTS정보가 없거나 잘못되었을 때 사용할 가짜 PTS

    this->endOfDecoding = false;

    while (this->isReading) 
    {
        {
            std::unique_lock<std::mutex> lock(this->bufferMutex);
            this->bufferCondVar.wait(lock, [this] {
                return this->packetBuffer.empty() == false &&
                    (this->packetBuffer.size() >= this->CAN_POP_RTSP_PACKET_BUFFER_SIZE || this->isReading == false) ||
                    this->endOfDecoding == true;
                });
        }
        if (this->endOfDecoding == true)
        {
            this->clearPacketBuffer();
            return;
        }

        AVFrame* frame = av_frame_alloc();

        //패킷버퍼에서 꺼내고 디코딩하는 것을 반복
        while (true)
        {
            if (this->endOfDecoding == true)
            {
                this->clearPacketBuffer();
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
                printf(">>>>패킷 꺼냄 남은 패킷:%zu개 \n", this->packetBuffer.size());
                this->bufferRrogress_percent = this->packetBuffer.size() / (double)this->MAX_RTSP_PACKET_BUFFER_SIZE * 100;
                bufferCondVar.notify_all();
            }

            if (packet->stream_index != this->video_stream_idx)
            {
                av_packet_unref(packet);
                av_packet_free(&packet);
                continue;
            }

            //패킷에 pts 정보가 없는 경우 임의로 넣음
            if (packet->pts == AV_NOPTS_VALUE)
            {
                packet->pts = packet->dts / this->dtsIncrement;
            }
            if (avcodec_send_packet(this->video_dec_ctx, packet) >= 0)
            {
                while (true)
                {
                    if (this->endOfDecoding == true)
                    {
                        av_packet_unref(packet);
                        av_packet_free(&packet);
                        this->clearPacketBuffer();
                        return;
                    }

                    int ret = avcodec_receive_frame(this->video_dec_ctx, frame);
                    if (ret < 0)
                    {
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
                        if (frame->pts == AV_NOPTS_VALUE)
                        {
                            frame->pts = frame->best_effort_timestamp;
                        }

                        if (isFirstFrameRendered == false)
                        {
                            this->directx11Renderer->Render(frame);
                            isFirstFrameRendered = true;
                            firstFramePts = frame->pts;
                            av_frame_unref(frame);
                            steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                            firstFrameRenderTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                        }
                        else
                        {
                            steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                            //현 프레임의 목표 랜더 시각 : 첫랜더 시각 + (현 프레임의 pts - 첫 프레임의 pts) * 타임베이스
                            int64_t targetRenderTime_ms = firstFrameRenderTime_ms + (frame->pts - firstFramePts) * this->videoTimeBase_ms;
                            //목표 랜더 시각과 현 시각의 차이 계산 : 현 시각 - 목표 시각
                            int64_t gapOfCurrentAndTargetTime_ms = now_ms - targetRenderTime_ms;
                            int64_t remainTimeToRender_ms = -gapOfCurrentAndTargetTime_ms;


                            if (remainTimeToRender_ms >= 0)
                            {//프레임 재생 지연
                                std::this_thread::sleep_for(std::chrono::milliseconds(remainTimeToRender_ms));

                                //directX11 랜더링
                                this->directx11Renderer->Render(frame);
                                printf("         render frame pts : %lld\n", frame->pts);
                            }
                            else 
                            {// 시간이 지났으므로 프레임 버림
                                printf("         dump frame pts : %lld    delayed %lld\n", frame->pts, -remainTimeToRender_ms);
                            }
                            av_frame_unref(frame);
                        }
                    }
                }
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
            av_frame_unref(frame);
        }

        av_frame_free(&frame);
    }

    //버퍼에 남아있는 패킷 처리
    this->clearPacketBuffer();

    avcodec_free_context(&this->video_dec_ctx);
}


































































































void Player::openFileStream(const char* filePath)
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    if (filePath != nullptr)
    {
        this->inputSourcePath = filePath;
    }

    int ret = 0;

    avformat_network_init();

    if (avformat_open_input(&this->formatContext, this->inputSourcePath, NULL, NULL) < 0) {
        fprintf(stderr, "영상 소스 열기 실패 %s\n", this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }

    if (avformat_find_stream_info(this->formatContext, NULL) < 0) {
        fprintf(stderr, "스트림 정보 찾기 실패 \n");
        avformat_close_input(&this->formatContext);
        this->isStreamSourceOpen = false;
        return;
    }

    ret = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "%s 스트림을 찾을 수 없음 '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), this->inputSourcePath);
        this->isStreamSourceOpen = false;
        return;
    }

    this->video_stream_idx = ret;
    this->videoStream = this->formatContext->streams[video_stream_idx];

    //CUDA 지원이 되는 포멧이면 CUDA 지원 코덱 가져오기
    if (this->IsCudaSupportedCodec(this->videoStream->codecpar->codec_id) == true)
    {
        this->videoDecoder = this->GetCudaCodecById(this->videoStream->codecpar->codec_id);
    }
    else
    {
        this->videoDecoder = avcodec_find_decoder(this->videoStream->codecpar->codec_id);
    }

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
        this->isStreamSourceOpen = false;
        return;
    }
    this->video_dec_ctx = avcodec_alloc_context3(this->videoDecoder);
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


    this->isStreamSourceOpen = true;

    this->swsCtx = sws_getContext(
        this->width, this->height, this->pix_fmt,
        this->width, this->height, AV_PIX_FMT_RGB24 /*AV_PIX_FMT_RGB24*/, /*AV_PIX_FMT_RGBA*/
        SWS_BILINEAR, NULL, NULL, NULL);

    int result = av_seek_frame(this->formatContext, video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(this->video_dec_ctx);
    if (this->onVideoSizeCallbackFunction != nullptr)
    {
        this->onVideoSizeCallbackFunction(this->width, this->height);
    }

    this->directx11Renderer = new DirectX11Renderer(this->hwnd_);
    if (this->directx11Renderer->Init(this->width, this->height) == false)
    {
        delete this->directx11Renderer;
    }
}


/// <summary>
/// 패킷을 읽어서 패킷Q에 삽입한다<br/>
/// 우선 임시버퍼에 GOP 단위만큼만 읽어들이고 GOP단위 만큼 DTS 순으로 정렬한 후 패킷 버퍼에 삽입한다.<br/>
/// </summary>
void Player::readThreadTask()
{
    std::vector<AVPacket*> tempBuffer;//온전한 GOP 단위의 패킷 묶음을 받는 목적의 임시 버퍼(I프레임 ~ 다음 I프레임 직전)
    bool isFirstKeyFrameFound = false;
    this->playStarted = false;
    this->isReading = true;
    this->dtsIncrement = 0;
    this->playStartedDts = 0;
    this->lastestPacketDts = 0;
    while (this->isReading == true) 
    {
        //리딩일시정지플래그 확인
        {
            std::unique_lock<std::mutex> readingMutexLock(this->readingPauseMutex);
            this->isWaitingAfterCommand = false;
            this->readingPauseCondVar.notify_all();
            //printf("    리드가 노티파이\n");
            this->readingPauseCondVar.wait(readingMutexLock, [this] { 
                return (this->isReadingPaused == false || this->isReading == false); 
                });
        }

        if (this->isReading == false)
        {
            break;
        }

        //seek 요청이 있었다면 기존에 읽은 패킷 확실히 다 지우기
        if (this->isToDumpPrevPacket == true)
        {
            for (AVPacket* p : tempBuffer)
            {
                av_packet_unref(p);
                av_packet_free(&p);
            }
            tempBuffer.clear();
            this->isToDumpPrevPacket = false;
        }

        AVPacket* packet = av_packet_alloc();
        //printf("푸시푸시푸시.................\n");
        //스트림에서 패킷 읽는데 성공한 경우
        if (av_read_frame(this->formatContext, packet) >= 0) 
        {
            //dts정보가 비정상이면 버리기
            if (packet->dts < 0)
            {
                av_packet_unref(packet);
                av_packet_free(&packet);
                continue;
            }

            //리드 시작 타이밍인 경우
            if (this->playStarted == false)
            {
                this->playStartedDts = packet->dts;//시작 DTS 기록
                this->playStarted = true;

                //프로그래스바에서 몇%에 해당하는 시간인지 계산
                double progressPercent = (double)packet->dts * this->videoTimeBase_ms / this->duration_ms * 100.0;
            }
            else
            {
                this->lastestPacketDts = packet->dts;//마지막으로 읽은 패킷의 DTS 갱신

                //프로그래스바에서 몇%에 해당하는 시간인지 계산
                double progressPercent = (double)packet->dts * this->videoTimeBase_ms / this->duration_ms * 100.0;
                this->bufferRrogress_percent = progressPercent;
            }

            //첫 키프레임 발견 전 일반 프레임
            if (isFirstKeyFrameFound == false && 
                packet->flags != AV_PKT_FLAG_KEY)
            {
                //키프레임 없는 일반 프레임은 의미가 없으므로 버림
                av_packet_unref(packet);
                av_packet_free(&packet);
            }
            //첫 키프레임 발견
            else if (isFirstKeyFrameFound == false && 
                packet->flags == AV_PKT_FLAG_KEY)
            {
                printf("첫 번 째 키프레임\n");
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
                printf("두 번 째 키프레임\n");

                //GOP 정렬(첫I부터 두번째I전까지)
                std::sort(tempBuffer.begin(), tempBuffer.end(), [](AVPacket* a, AVPacket* b) {
                    return a->dts < b->dts;
                    });

                int64_t prevDts = -1;
                int64_t prevCurrentGap = 0;
                //정렬된 패킷들을 진짜 패킷 버퍼에 삽입
                for (AVPacket* p : tempBuffer)
                {
                    //DTS 최소 간격 계산 : PTS가 없을 때 임시 PTS 계산을 위함
                    if (prevDts >= 0)
                    {
                        prevCurrentGap = p->dts - prevDts;
                        if (prevCurrentGap > 0 && 
                            prevCurrentGap <= this->dtsIncrement ||
                            this->dtsIncrement == 0)
                        {
                            this->dtsIncrement = prevCurrentGap;
                        }
                    }
                    prevDts = p->dts;

                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] {
                            return (this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false);
                        });
                    if (this->isReading == false)
                    {
                        av_packet_unref(p);
                        av_packet_free(&p);
                        continue;
                    }

                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("push packet DTS : %lld    buffer size : %zu\n", p->dts, currentPacketBufferSize);

                        if (currentPacketBufferSize >= this->CAN_POP_PACKET_BUFFER_SIZE)
                        {
                            this->bufferCondVar.notify_all();
                        }
                    }
                }
                tempBuffer.clear();

                //두번째 키프레임을 첫번째 키프레임으로 취급
                tempBuffer.push_back(packet);
            }

            {
                std::lock_guard<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.size() >= this->CAN_POP_PACKET_BUFFER_SIZE) {
                    this->bufferCondVar.notify_all();
                }
            }
            
        }
        else //에러 또는 파일이 끝난 경우(av_read_frame 함수가 0 미만의 값을 리턴한 경우)
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
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    this->bufferCondVar.wait(lock, [this] { return this->packetBuffer.size() < this->MAX_PACKET_BUFFER_SIZE || this->isReadingPaused == true || this->isReading == false; });
                    
                    size_t currentPacketBufferSize = this->packetBuffer.size();
                    if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->packetBuffer.push(p);
                        printf("err_ push packet DTS : %lld    buffer size : %zu\n", p->dts, currentPacketBufferSize);
                    }
                    
                    if (currentPacketBufferSize >= this->MAX_PACKET_BUFFER_SIZE)
                    {
                        this->bufferCondVar.notify_all();
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
                size_t currentPacketBufferSize = this->packetBuffer.size();
                if (currentPacketBufferSize < this->MAX_PACKET_BUFFER_SIZE)
                {
                    //디코딩 쓰레드에게 끝을 알리기 위한 가짜 패킷 삽입
                    AVPacket* endPacket = av_packet_alloc();
                    endPacket->flags = PKT_END;
                    this->packetBuffer.push(endPacket);
                    printf("last_ push packet DTS : %lld    buffer size : %zu\n", endPacket->dts, currentPacketBufferSize);
                }
            }
            
            this->bufferCondVar.notify_all();
            continue;
        }
    }

    for (AVPacket* p : tempBuffer)
    {
        av_packet_unref(p);
        av_packet_free(&p);
    }
    tempBuffer.clear();

    this->playStarted = false;
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();

    avcodec_free_context(&this->video_dec_ctx);
    avformat_close_input(&this->formatContext);
    avformat_network_deinit();
    sws_freeContext(this->swsCtx);
}



/// <summary>
/// 디코딩 후 랜더링,
/// 버퍼가 CAN_POP_PACKET_BUFFER_SIZE이상 차면 시작 가능,
/// 한 번 시작하면 버퍼가 빌 때 까지 계속 디코딩,
/// </summary>
void Player::videoDecodeAndRenderThreadTask()
{
    this->isFirstFrameRendered = false;//첫 프레임이 랜더링 되었냐 여부
    this->isTimeToSkipFrame = false;
    int64_t firstFrameRenderTime_ms = 0;//첫 프레임이 랜더링된 시각
    int64_t firstFramePts = 0;//첫 재생 프레임의 pts
    int64_t nextFrameRenderTime_ms = 0;//다음 프레임 랜더링 예정 시각
    int64_t fakePts = -1;//frame에 PTS정보가 없거나 잘못되었을 때 사용할 가짜 PTS
    bool isNoPts = false;//패킷에 pts 정보가 없을 때

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
            while (true)
            {
                AVPacket* packet = nullptr;
                std::unique_lock<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.empty() == true)
                {
                    break;
                }
                packet = this->packetBuffer.front();
                this->packetBuffer.pop();
                av_packet_unref(packet);
                av_packet_free(&packet);
                bufferCondVar.notify_all();////
            }
            return;
        }

        
        //디코딩일시정지플래그 확인
        {
            std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
            this->isWaitingAfterCommand = false;
            this->decodingPauseCondVar.notify_all();
            this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] {
                if (this->isDecodingPaused == true)
                    this->isFirstFrameRendered = false;
                return this->isDecodingPaused == false || this->endOfDecoding == true;
                });
        }
        if (this->endOfDecoding == true)
        {
            while (true)
            {
                AVPacket* packet = nullptr;
                std::unique_lock<std::mutex> lock(this->bufferMutex);
                if (this->packetBuffer.empty() == true)
                {
                    break;
                }
                packet = this->packetBuffer.front();
                this->packetBuffer.pop();
                av_packet_unref(packet);
                av_packet_free(&packet);
                bufferCondVar.notify_all();////
            }
            return;
        }

        while (true) 
        {
            //디코딩일시정지플래그 확인
            {
                std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
                this->isWaitingAfterCommand = false;
                this->decodingPauseCondVar.notify_all();
                this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] {
                    if (this->isDecodingPaused == true)
                        this->isFirstFrameRendered = false;
                    return this->isDecodingPaused == false || this->endOfDecoding == true;
                    });
            }
            if (this->endOfDecoding == true)
            {
                while (true)
                {
                    AVPacket* packet = nullptr;
                    std::unique_lock<std::mutex> lock(this->bufferMutex);
                    if (this->packetBuffer.empty() == true)
                    {
                        break;
                    }
                    packet = this->packetBuffer.front();
                    this->packetBuffer.pop();
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    bufferCondVar.notify_all();////
                }
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
                av_packet_unref(packet);
                av_packet_free(&packet);
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
                this->directx11Renderer->ClearScreen();

                this->endOfDecoding = true;

                auto t = new std::thread([this]() {
                    this->stop();
                    });

                fprintf(stderr, "   end\n");
                break;
            }


            AVFrame* frame = av_frame_alloc();

            //패킷에 pts 정보가 없는 경우 임의로 넣음
            if (packet->pts == AV_NOPTS_VALUE)
            {
                packet->pts = packet->dts / this->dtsIncrement;
            }
            if (avcodec_send_packet(video_dec_ctx, packet) >= 0)
            {
                while (true)
                {
                    //디코딩일시정지플래그 확인
                    {
                        std::unique_lock<std::mutex> decodingPauseMutexLock1(this->decodingPauseMutex);
                        this->isWaitingAfterCommand = false;
                        this->decodingPauseCondVar.notify_all();
                        this->decodingPauseCondVar.wait(decodingPauseMutexLock1, [this] { 
                            if (this->isDecodingPaused == true)
                            {
                                this->isFirstFrameRendered = false;
                                this->isTimeToSkipFrame = true;
                            }
                            return this->isDecodingPaused == false || this->endOfDecoding == true; 
                            });
                    }
                    if (this->endOfDecoding == true)
                    {
                        av_packet_unref(packet);
                        av_packet_free(&packet);

                        while (true)
                        {
                            AVPacket* packet = nullptr;
                            std::unique_lock<std::mutex> lock(this->bufferMutex);
                            if (this->packetBuffer.empty() == true)
                            {
                                break;
                            }
                            packet = this->packetBuffer.front();
                            this->packetBuffer.pop();
                            av_packet_unref(packet);
                            av_packet_free(&packet);
                            bufferCondVar.notify_all();////
                        }
                        return;
                    }

                    //일시정지 되어서 다시 시작해야하는 경우
                    if (this->isTimeToSkipFrame == true)
                    {
                        while (avcodec_receive_frame(video_dec_ctx, frame) >= 0)
                        {
                            av_frame_unref(frame);
                            printf("dump frame pts:&lld", frame->pts);
                        }
                        this->isTimeToSkipFrame = false;
                        break;
                    }
                    
                    int ret = avcodec_receive_frame(video_dec_ctx, frame);
                    if (ret < 0)
                    {
                        av_frame_unref(frame);

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
                        // PTS가 없는 경우 best_effort_timestamp 사용
                        if (frame->pts == AV_NOPTS_VALUE)
                        {
                            frame->pts = frame->best_effort_timestamp;
                        }


                        if (this->isFirstFrameRendered == false)
                        {
                            this->directx11Renderer->Render(frame);
                            printf("         render frame pts : %lld\n", frame->pts);
                            this->isFirstFrameRendered = true;
                            firstFramePts = frame->pts;
                            steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                            firstFrameRenderTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                        }
                        else
                        {
                            steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                            //현 프레임의 목표 랜더 시각 : 첫랜더 시각 + (현 프레임의 pts - 첫 프레임의 pts) * 타임베이스
                            int64_t targetRenderTime_ms = firstFrameRenderTime_ms + (frame->pts - firstFramePts) * this->videoTimeBase_ms;
                            //목표 랜더 시각과 현 시각의 차이 계산 : 현 시각 - 목표 시각
                            int64_t gapOfCurrentAndTargetTime_ms = now_ms - targetRenderTime_ms;
                            int64_t remainTimeToRender_ms = -gapOfCurrentAndTargetTime_ms;

                            if (remainTimeToRender_ms >= 0)
                            {//프레임 재생 지연
                                std::this_thread::sleep_for(std::chrono::milliseconds(remainTimeToRender_ms));

                                //directX11 랜더링
                                this->directx11Renderer->Render(frame);

                                printf("         render frame pts : %lld\n", frame->pts);
                            }
                            else
                            {// 시간이 지났으므로 프레임 버림
                                printf("         dump frame pts : %lld\n", frame->pts);
                            }
                        }

                        //프로그래스바 갱신
                        this->progress_percent = (double)frame->pts * videoTimeBase_ms / this->duration_ms * 100.0;
                        fprintf(stderr, "         pts : %lld         프로그래스 : %lld \n", frame->pts, this->progress_percent);
                        
                    }
                    av_frame_unref(frame);

                }
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
            av_frame_unref(frame);
            av_frame_free(&frame);
        }

        bufferCondVar.notify_all();
    }

    while(true)
    {
        AVPacket* packet = nullptr;
        std::unique_lock<std::mutex> lock(this->bufferMutex);
        if (this->packetBuffer.empty() == true)
        {
            break;
        }
        packet = this->packetBuffer.front();
        this->packetBuffer.pop();
        av_packet_unref(packet);
        av_packet_free(&packet);
        bufferCondVar.notify_all();////
    }
}












































































































































//프로그래스 체크
void Player::progressCheckingThreadTask()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (this->decodeAndRenderThread != nullptr &&
            this->decodeAndRenderThread->joinable() == true)
        {
            if (this->isPaused == false)
            {
                this->onVideoProgressCallback(this->progress_percent);


            }

            this->bufferCondVar.notify_all();
        }

        if (this->readThread != nullptr &&
            this->readThread->joinable() == true)
        {
            if (this->onBufferProgressCallback != nullptr)
            {
                //버퍼 프로그래스바 갱신
                this->onBufferProgressCallback(this->bufferRrogress_percent);
            }
        }

        if (this->readRtspThread != nullptr &&
            this->readRtspThread->joinable() == true ||
            this->decodeAndRenderRtspThread != nullptr &&
            this->decodeAndRenderRtspThread->joinable() == true)
        {
            if (this->onBufferProgressCallback != nullptr)
            {
                //버퍼 프로그래스바 갱신
                this->onBufferProgressCallback(this->bufferRrogress_percent);
                this->bufferCondVar.notify_all();
            }
        }
    }
}




















































































































int Player::playRtsp(const char* rtspPath)
{
    //std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    this->stopRtsp();
    this->openRtspStream(rtspPath);
    this->startReadRtspThread();
    this->startDecodeAndRenderRtspThread();

    if (this->onStartCallbackFunction != nullptr)
    {
        this->onStartCallbackFunction();
    }
    return 0;
}

int Player::stopRtsp()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);

    //디코딩 쓰레드 종료
    this->endOfDecoding = true;
    this->bufferCondVar.notify_all();
    this->decodingPauseCondVar.notify_all();
    if (this->decodeAndRenderRtspThread != nullptr &&
        this->decodeAndRenderRtspThread->joinable() == true)
    {
        this->decodeAndRenderRtspThread->join();
    }

    //리딩 쓰레드 종료
    this->isReading = false;
    this->bufferCondVar.notify_all();
    this->readingPauseCondVar.notify_all();
    if (this->readRtspThread != nullptr &&
        this->readRtspThread->joinable() == true)
    {
        this->readRtspThread->join();
    }

    //패킷버퍼 clear
    {
        std::unique_lock<std::mutex> lock(bufferMutex);
        while (!this->packetBuffer.empty())
        {
            AVPacket* packet = this->packetBuffer.front();
            av_packet_unref(packet);
            av_packet_free(&packet);
            delete packet;
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
    this->endOfDecoding = false;
    this->isReading = true;

    if (this->directx11Renderer != nullptr)
    {
        this->directx11Renderer->Cleanup();
        delete this->directx11Renderer;
        this->directx11Renderer = nullptr;
    }

    printf("stop return\n");
    //콜백 호출
    if (this->onStopCallbackFunction != nullptr)
    {
        this->onStopCallbackFunction();
    }
    return 0;
}



























































































int Player::play(const char* filePath)
{
    if (this->isPaused == true)
    {
        this->resume();
    }
    else
    {
        this->stop();
        this->openFileStream(filePath);
        this->startReadThread();
        this->startDecodeAndRenderThread();
    }

    printf("play return\n");
    if (this->onStartCallbackFunction != nullptr)
    {
        this->onStartCallbackFunction();
    }
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

    printf("pause return\n");
    if (this->onPauseCallbackFunction != nullptr)
    {
        this->onPauseCallbackFunction();
    }
    return 0;
}

int Player::resume()
{
    std::lock_guard<std::mutex> decodingPauseMutexLock(this->commandMutex);
    this->isDecodingPaused = false;
    this->decodingPauseCondVar.notify_all();

    this->isPaused = false;

    printf("resume return\n");
    if (this->onResumeCallbackFunction != nullptr)
    {
        this->onResumeCallbackFunction();
    }
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
            AVPacket* packet = this->packetBuffer.front();
            av_packet_unref(packet);
            av_packet_free(&packet);
            delete packet;
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

    if (this->directx11Renderer != nullptr)
    {
        this->directx11Renderer->Cleanup();
        delete this->directx11Renderer;
        this->directx11Renderer = nullptr;
    }

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
        if (this->onSeekCallbackFunction != nullptr)
        {
            this->onSeekCallbackFunction();
        }
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
        avcodec_flush_buffers(this->video_dec_ctx);

        //이전에 읽은 패킷은 비우도록
        this->isToDumpPrevPacket = true;

        //리드 쓰레드 재개
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

    }
    else
    {
        if (this->decodeAndRenderThread->joinable() == false || this->readThread->joinable() == false)
        {
            if (this->onSeekCallbackFunction != nullptr)
            {
                this->onSeekCallbackFunction();
            }
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
                AVPacket* packet = this->packetBuffer.front();
                av_packet_unref(packet);
                av_packet_free(&packet);
                delete packet;
                this->packetBuffer.pop();
            }
        }


        this->isFirstFrameRendered = false;

        //SEEK 명령
        int result = av_seek_frame(this->formatContext, video_stream_idx, seekTimeStamp, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(this->video_dec_ctx);
        
        //이전에 읽은 패킷은 비우도록
        this->isToDumpPrevPacket = true;

        //리드 쓰레드 재개
        this->isReadingPaused = false;
        this->readingPauseCondVar.notify_all();

        //디코드,랜더 쓰레드 재개
        this->isDecodingPaused = false;
        this->decodingPauseCondVar.notify_all();
        this->isPaused = false;
    }

    if (this->onSeekCallbackFunction != nullptr)
    {
        this->onSeekCallbackFunction();
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

void Player::RegisterOnSeekCallback(OnSeekCallbackFunction callback)
{
    this->onSeekCallbackFunction = callback;
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


void Player::RegisterOnVideoSizeCallback(OnVideoSizeCallbackFunction callback)
{
    this->onVideoSizeCallbackFunction = callback;
}










































void Player::DrawDirectXTestRectangle()
{
    if (this->directx11Renderer == nullptr)
    {
        return;
    }
    this->directx11Renderer->RenderEmptyRect();
}




























































/// <summary>
/// CUDA를 지원하는 코덱타입인지
/// </summary>
/// <param name="codecId"></param>
/// <returns></returns>
bool Player::IsCudaSupportedCodec(AVCodecID codecId) {
    if (codecId == AV_CODEC_ID_H264 ||
        codecId == AV_CODEC_ID_HEVC ||
        codecId == AV_CODEC_ID_MPEG1VIDEO ||
        codecId == AV_CODEC_ID_MPEG2VIDEO ||
        codecId == AV_CODEC_ID_MPEG4 ||
        codecId == AV_CODEC_ID_VC1 ||
        codecId == AV_CODEC_ID_VP8 ||
        codecId == AV_CODEC_ID_VP9) 
    {
        return true;
    }
    return false;
}

/// <summary>
/// CUDA 코덱 가져오기
/// </summary>
/// <param name="codecId"></param>
/// <returns>CUDA코덱</returns>
const AVCodec* Player::GetCudaCodecById(AVCodecID codecId)
{
    if (codecId == AV_CODEC_ID_H264)
    {
        return avcodec_find_decoder_by_name("h264_cuvid");
    }
    else if (codecId == AV_CODEC_ID_HEVC)
    {
        return avcodec_find_decoder_by_name("hevc_cuvid");
    }
    else if (codecId == AV_CODEC_ID_MPEG1VIDEO)
    {
        return avcodec_find_decoder_by_name("mpeg1_cuvid");
    }
    else if (codecId == AV_CODEC_ID_MPEG2VIDEO)
    {
        return avcodec_find_decoder_by_name("mpeg2_cuvid");
    }
    else if (codecId == AV_CODEC_ID_MPEG4)
    {
        return avcodec_find_decoder_by_name("mpeg4_cuvid");
    }
    else if (codecId == AV_CODEC_ID_VC1)
    {
        return avcodec_find_decoder_by_name("vc1_cuvid");
    }
    else if (codecId == AV_CODEC_ID_VP8)
    {
        return avcodec_find_decoder_by_name("vp8_cuvid");
    }
    else if (codecId == AV_CODEC_ID_VP9)
    {
        return avcodec_find_decoder_by_name("vp9_cuvid");
    }
}



























Player::Player() 
{

}

Player::Player(HWND  hWnd)
{
    this->hwnd_ = hWnd;
}

void Player::Cleanup() 
{
    if (this->video_dec_ctx) avcodec_free_context(&this->video_dec_ctx);
    if (this->formatContext) avformat_close_input(&this->formatContext);
    if (this->swsCtx) sws_freeContext(this->swsCtx);
    avformat_network_deinit();
}