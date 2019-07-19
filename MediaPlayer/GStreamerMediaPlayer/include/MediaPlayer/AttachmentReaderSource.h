/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ATTACHMENTREADERSOURCE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ATTACHMENTREADERSOURCE_H_

#include <memory>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

#include "MediaPlayer/BaseStreamSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

class AttachmentReaderSource : public BaseStreamSource {
public:
    /**
     * Creates an instance of the @c AttachmentReaderSource and installs the source within the GStreamer pipeline.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param attachmentReader The @c AttachmentReader from which to create the pipeline source from.
     * @param audioFormat The audioFormat to be used when playing raw PCM data.
     * @param repeat A parameter indicating whether to play from the source in a loop.
     * @return An instance of the @c AttachmentReaderSource if successful else a @c nullptr.
     */
    static std::unique_ptr<AttachmentReaderSource> create(
        PipelineInterface* pipeline,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* audioFormat,
        bool repeat);

    ~AttachmentReaderSource();

    /// @name Overridden SourceInterface methods.
    /// @{
    bool isPlaybackRemote() const override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param attachmentReader The @c AttachmentReader from which to create the pipeline source from.
     * @param repeat A parameter indicating whether to play from the source in a loop.
     */
    AttachmentReaderSource(
        PipelineInterface* pipeline,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        bool repeat);

    /// @name Overridden BaseStreamSource methods.
    /// @{
    bool isOpen() override;
    void close() override;
    gboolean handleReadData() override;
    gboolean handleSeekData(guint64 offset) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override{};
    /// @}

private:
    /// The @c AttachmentReader to read audioData from.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_reader;

    /// Indicates whether to play from the audio source in a loop.
    const bool m_repeat;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ATTACHMENTREADERSOURCE_H_
