// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"


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
}

#define INBUF_SIZE 4096

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

extern "C" __declspec(dllexport) int Add(int a, int b) {
    unsigned version = avformat_version();
    printf("libavformat version: %u.%u.%u\n", version >> 16, (version >> 8) & 0xFF, version & 0xFF);
    return a + b;
}

/// <summary>
/// 디먹스, 디코딩 예제
/// </summary>
extern "C" __declspec(dllexport) void RunDecodeExample1() {
    avformat_network_init();

    AVFormatContext* formatContext = nullptr;
    const AVCodec* videoDecoder = NULL;
    const AVCodec* audioDecoder = NULL;
    AVCodecContext* codecContext = NULL;
    AVCodecContext* video_dec_ctx = NULL, * audio_dec_ctx;
    AVPacket* packet;
    AVFrame* frame;

    AVStream* videoStream;
    int width, height;
    enum AVPixelFormat pix_fmt;
    uint8_t* video_dst_data[4] = { NULL };
    int video_dst_linesize[4];
    int video_dst_bufsize;
    int video_stream_idx = -1, audio_stream_idx = -1;




    const char* filename = "video.avi";


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

        ret = av_image_alloc(video_dst_data, video_dst_linesize,
            width, height, pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
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

    packet = av_packet_alloc();
    if (!packet)
    {
        return;
    }

    


    while (av_read_frame(formatContext, packet) >= 0) {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        if (packet->stream_index == video_stream_idx)
        {
            ret = avcodec_send_packet(video_dec_ctx, packet);
            if (ret < 0) {
                fprintf(stderr, "Error submitting a packet for decoding\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(video_dec_ctx, frame);
                if (ret < 0) {
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

                    fprintf(stderr, "Error during decoding\n");
                    break;
                }

                // write the frame data to output file
                if (video_dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO) 
                {
                    fprintf(stderr, "비디오 패킷 디코디드\n");
                }
                else
                {
                    fprintf(stderr, "패킷 디코디드\n");
                }


                av_frame_unref(frame);
                if (ret < 0)
                    break;
            }
        }
        else if (packet->stream_index == audio_stream_idx)
        {
        }

        av_packet_unref(packet);
        /*if (ret < 0)
            break;*/
    }


end:


    avcodec_free_context(&video_dec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avformat_close_input(&formatContext);
    av_free(video_dst_data[0]);

    fprintf(stderr, "끝\n");

    return;
}




extern "C" __declspec(dllexport) void RunDecodeExample2() {
    /*avformat_network_init();

    AVFormatContext* formatContext = nullptr;*/

    const AVCodec* codec;
    AVCodecParserContext* parser; // 스트림(비디오 또는 오디오)의 코덱 데이터를 분석(parse)하기 위한 구조체
    AVCodecContext* codecContext = NULL;
    AVPacket* packet;
    AVFrame* frame;
    
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    FILE* file;
    const char* filename = "video.avi";
    size_t   data_size;
    uint8_t* data;


    int ret;
    int eof;

    fopen_s(&file,filename, "rb");
    if (file == nullptr) {
        perror("File opening failed");
        return;
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        fprintf(stderr, "Could not allocate video codec context\n");
        return;
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        return;
    }

    packet = av_packet_alloc();
    if (!packet)
    {
        return;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }


    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    do {
        data_size = fread(inbuf, 1, INBUF_SIZE, file);
        if (ferror(file))
        {
            break;
        }
        eof = !data_size;
        data = inbuf;

        while (data_size > 0 || eof) {
            ret = av_parser_parse2(parser, codecContext, &packet->data, &packet->size,
                data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                return;
            }
            data += ret;
            data_size -= ret;

            if (packet->size)
            {
                char buf[1024];
                int ret;
                ret = avcodec_send_packet(codecContext, packet);
                if (ret < 0) {
                    fprintf(stderr, "Error sending a packet for decoding\n");
                    return;
                }
                while (ret >= 0) 
                {
                    ret = avcodec_receive_frame(codecContext, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    {
                        return;
                    }
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        return;
                    }

                }
            }
                
            else if (eof)
                break;
        }

    } while (!eof);



    fclose(file);
    av_parser_close(parser);
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
}