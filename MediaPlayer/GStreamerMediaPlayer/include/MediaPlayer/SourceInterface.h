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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEINTERFACE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEINTERFACE_H_

#include "MediaPlayer/PipelineInterface.h"

#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * Interface to request operations on an audio source.
 */
class SourceInterface : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor, which also constructs RequiresShutdown.
     *
     * @param className The name of the class to be passed to @c RequiresShutdown.
     */
    SourceInterface(const std::string& className) : RequiresShutdown(className) {
    }

    /**
     * Destructor.
     */
    virtual ~SourceInterface() = default;

    /**
     * Internally, a source may need additional processing after EOS is reached.
     * This function will process that data.
     *
     * @return A boolean indicating whether the process operation was successful.
     */
    virtual bool handleEndOfStream() = 0;

    /**
     * Internally, a source may have additional data after processing an EOS.
     * This function indicates whether there is additional data, and should be called
     * after @c handleEndOfStream().
     *
     * @return A boolean indicating whether the source has additional data to be played.
     */
    virtual bool hasAdditionalData() = 0;

    /**
     * Perform preprocessing  of the source. Must be called before reading from the source.
     */
    virtual void preprocess() = 0;

    /**
     * Indicates whether a source is local or remote from the perspective of the MediaPlayer (e.g. playing out of the
     * SDS is local, playing a URL is remote).
     *
     * @return A boolean indicating whether the source is from a remote or local source
     */
    virtual bool isPlaybackRemote() const = 0;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEINTERFACE_H_
