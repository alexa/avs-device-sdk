/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/dict.h>
#include <libswresample/swresample.h>
}

#include "AndroidSLESMediaPlayer/FFmpegDeleter.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

template <>
void FFmpegDeleter<AVCodecContext>::operator()(AVCodecContext* context) {
    avcodec_free_context(&context);
}

template <>
void FFmpegDeleter<AVDictionary>::operator()(AVDictionary* dictionary) {
    av_dict_free(&dictionary);
}

template <>
void FFmpegDeleter<AVInputFormat>::operator()(AVInputFormat* format) {
    // Empty by design. The @c format points to a static object.
}

template <>
void FFmpegDeleter<AVIOContext>::operator()(AVIOContext* context) {
    av_free(context->buffer);
    av_free(context);
}

template <>
void FFmpegDeleter<AVFormatContext>::operator()(AVFormatContext* context) {
    avformat_close_input(&context);
}

template <>
void FFmpegDeleter<AVPacket>::operator()(AVPacket* packet) {
    av_packet_unref(packet);
}

template <>
void FFmpegDeleter<AVFrame>::operator()(AVFrame* frame) {
    av_frame_free(&frame);
}

template <>
void FFmpegDeleter<SwrContext>::operator()(SwrContext* context) {
    swr_free(&context);
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
