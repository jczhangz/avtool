//
// Created by zz on 2023/8/15.
//
#include "iostream"
#include "av_library.h"
#include "fstream"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
}

int frame2Image(AVFrame *frame, enum AVCodecID codecId, uint8_t *outBuf, size_t outBufSize);

void saveFrame2Jpg(AVFrame *frame, const char *imgPath);

bool fileExists(const char *filePath){
    std::ifstream file(filePath);
    return file.good();
}

int captureImage(const char *videoPath, const char *imagePath, unsigned int frameIndex) {
    bool isFindFile = fileExists(videoPath);
    if(!isFindFile){
        fprintf(stderr,"video file not find");
        return -1;
    }


    AVFormatContext *formatContext = nullptr;
    int ret = 0;
    ret = avformat_open_input(&formatContext, videoPath, NULL, NULL);

    if (ret < 0) {
        fprintf(stderr, "open video fail");
        std::cout << "open video fail \n";
        exit(-1);
    }

    int videoStreamIndex = -1;
    AVCodecContext *codecContext = nullptr;
    AVCodecParameters *codecParameters = nullptr;

    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            codecParameters = formatContext->streams[i]->codecpar;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        fprintf(stderr, "Could not find video stream");
        avformat_close_input(&formatContext);
        return -1;
    }

    int64_t totalFrames = formatContext->streams[videoStreamIndex]->nb_frames;
    if (frameIndex > totalFrames) {
        fprintf(stderr, "frameIndex greater totalFrames");
        avformat_close_input(&formatContext);
        return -1;
    }

    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        fprintf(stderr, "Could not find avcodec");
        avformat_close_input(&formatContext);
        return -1;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        fprintf(stderr, "avcodec alloc context fail");
        avformat_close_input(&formatContext);
        return -1;
    }
    ret = avcodec_parameters_to_context(codecContext, codecParameters);

    if (ret < 0) {
        fprintf(stderr, "codec parameters to context fail");
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1;
    }
    ret = avcodec_open2(codecContext, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "codec open fail");
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int currentFrame = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            currentFrame = currentFrame + 1;
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    //抽取指定帧位置 为图片
                    if (currentFrame == frameIndex) {
                        saveFrame2Jpg(frame, imagePath);
                    }
                }
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    return ret;
}


void saveFrame2Jpg(AVFrame *frame, const char *imgPath) {
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA, frame->width, frame->height, 64);

    uint8_t *buf = (uint8_t *) av_malloc(bufSize);

    frame2Image(frame, AV_CODEC_ID_MJPEG, buf, bufSize);

    auto f = fopen(imgPath, "wb+");

    if (f) {
        fwrite(buf, sizeof(uint8_t), bufSize, f);
        fclose(f);
    }
    av_free(buf);
}


int frame2Image(AVFrame *frame, enum AVCodecID codecId, uint8_t *outBuf, size_t outBufSize) {
    int ret = 0;
    AVPacket pkt;

    const AVCodec *codec = nullptr;
    AVCodecContext *codec_context = nullptr;
    AVFrame *rgbFrame = nullptr;
    uint8_t *buffer = nullptr;


    static struct SwsContext *swsContext;
//    av_packet_unref(&pkt);
    av_init_packet(&pkt);
//    pkt = av_packet_alloc();
    codec = avcodec_find_encoder(codecId);

    if (!codec) {
        fprintf(stderr, "avcodec find decoder fail");
        goto end;
    }

    if (!codec->pix_fmts) {
        fprintf(stderr, "unsupport pix format width codec");
        goto end;
    }

    codec_context = avcodec_alloc_context3(codec);
    codec_context->bit_rate = 30000000;
    codec_context->width = frame->width;
    codec_context->height = frame->height;
    codec_context->time_base.num = 1;
    codec_context->time_base.den = 25;
    codec_context->gop_size = 10;
    codec_context->max_b_frames = 0;
    codec_context->thread_count = 1;
    codec_context->pix_fmt = *codec->pix_fmts;

    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        fprintf(stderr, "avcodec open2 fail");
        goto end;
    }

    if (frame->format != codec_context->pix_fmt) {
        rgbFrame = av_frame_alloc();

        if (rgbFrame == nullptr) {
            fprintf(stderr, "av frame alloc fail");
            goto end;
        }

        swsContext = sws_getContext(frame->width, frame->height, (enum AVPixelFormat) frame->format, frame->width,
                                    frame->height, codec_context->pix_fmt, 1, NULL, NULL, NULL);

        if (!swsContext) {
            fprintf(stderr, "sws_getContext fail!");
            goto end;
        }

        int bufferSize = av_image_get_buffer_size(codec_context->pix_fmt, frame->width, frame->height, 1) * 2;
        buffer = (unsigned char *) av_malloc(bufferSize);

        if (buffer == nullptr) {
            fprintf(stderr, "buffer malloc fail");
            goto end;
        }

        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, codec_context->pix_fmt, frame->width,
                             frame->height, 1);

        if (sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize) <
            0) {
            fprintf(stderr, "sws_scale fail");
            goto end;
        }

        rgbFrame->format = codec_context->pix_fmt;
        rgbFrame->width = codec_context->width;
        rgbFrame->height = codec_context->height;
        ret = avcodec_send_frame(codec_context, rgbFrame);
    } else {
        ret = avcodec_send_frame(codec_context, frame);
    }

    if (ret < 0) {
        fprintf(stderr, "avcodec_send_frame fail");
        goto end;
    }

    ret = avcodec_receive_packet(codec_context, &pkt);
    if (ret < 0) {
        fprintf(stderr, "avcodec_receive_packet fail");
        goto end;
    }

    if (pkt.size > 0 && pkt.size <= outBufSize)
        memcpy(outBuf, pkt.data, pkt.size);
    ret = pkt.size;

    end:

    if (swsContext) {
        sws_freeContext(swsContext);
    }
    if (rgbFrame) {
        av_frame_unref(rgbFrame);
        av_frame_free(&rgbFrame);
    }
    if (buffer) {
        av_free(buffer);
    }
    av_packet_unref(&pkt);
    if (codec_context) {
        avcodec_close(codec_context);
        avcodec_free_context(&codec_context);
    }

    return ret;
}


