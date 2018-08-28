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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDELETER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDELETER_H_

struct AVInputFormat;
struct AVDictionary;
struct AVCodecContext;
struct AVIOContext;
struct AVFormatContext;
struct AVPacket;
struct AVFrame;
struct SwrContext;

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * Generic deleter class to free FFmpeg C structures.
 *
 * @tparam ObjectT The type of object to be deleted.
 */
template <typename ObjectT>
class FFmpegDeleter {
public:
    void operator()(ObjectT* object);
};

/// @name FFMpeg FFmpegDeleter specializations.
/// @{
using AVInputFormatDeleter = FFmpegDeleter<AVInputFormat>;
using AVDictionaryDeleter = FFmpegDeleter<AVDictionary>;
using AVContextDeleter = FFmpegDeleter<AVCodecContext>;
using AVIOContextDeleter = FFmpegDeleter<AVIOContext>;
using AVFormatContextDeleter = FFmpegDeleter<AVFormatContext>;
using AVPacketDeleter = FFmpegDeleter<AVPacket>;
using AVFrameDeleter = FFmpegDeleter<AVFrame>;
using SwrContextDeleter = FFmpegDeleter<SwrContext>;
/// @}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGDELETER_H_
