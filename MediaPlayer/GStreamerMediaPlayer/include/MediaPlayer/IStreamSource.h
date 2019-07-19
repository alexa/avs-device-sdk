/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ISTREAMSOURCE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ISTREAMSOURCE_H_

#include <iostream>
#include <memory>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

#include "MediaPlayer/BaseStreamSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

class IStreamSource : public BaseStreamSource {
public:
    /**
     * Create an IStream source.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param stream The @c std::istream from which to create the pipeline source.
     * @param repeat Whether the stream should be replayed until stopped.
     */
    static std::unique_ptr<IStreamSource> create(
        PipelineInterface* pipeline,
        std::shared_ptr<std::istream> stream,
        bool repeat);

    /**
     * Destructor.
     */
    ~IStreamSource() override;

private:
    /**
     * Constructor.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param stream The @c std::istream from which to create the pipeline source.
     * @param repeat Whether the stream should be replayed until stopped.
     */
    IStreamSource(PipelineInterface* pipeline, std::shared_ptr<std::istream> stream, bool repeat);

    /// @name Overridden SourceInterface methods.
    /// @{
    bool isPlaybackRemote() const override;
    bool hasAdditionalData() override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override{};
    /// @}

    /// @name Overridden BaseStreamSource methods.
    /// @{
    bool isOpen() override;
    void close() override;
    gboolean handleReadData() override;
    gboolean handleSeekData(guint64 offset) override;
    /// @}

private:
    /// The std::istream to read audioData from.
    std::shared_ptr<std::istream> m_stream;

    /// Play the stream over and over until told to stop.
    bool m_repeat;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ISTREAMSOURCE_H_
