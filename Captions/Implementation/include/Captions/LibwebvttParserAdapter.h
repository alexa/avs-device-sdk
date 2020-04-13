/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_LIBWEBVTTPARSERADAPTER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_LIBWEBVTTPARSERADAPTER_H_

#include <chrono>
#include <map>
#include <memory>

#include "CaptionParserInterface.h"
#include <Captions/CaptionData.h>
#include <Captions/CaptionFrame.h>
#include <Captions/CaptionLine.h>

namespace alexaClientSDK {
namespace captions {

/**
 * A singleton implementation of the @c CaptionParserInterface, specified to work with the libwebvtt parsing library
 * found at: https://github.com/alexa/webvtt
 */
class LibwebvttParserAdapter : public CaptionParserInterface {
public:
    /**
     * Return the singleton instance of @c LibwebvttParserAdapter.
     *
     * @return The singleton instance of @c LibwebvttParserAdapter.
     */
    static std::shared_ptr<LibwebvttParserAdapter> getInstance();

    /// @name CaptionParserInterface methods
    /// @{
    void parse(CaptionFrame::MediaPlayerSourceId captionId, const CaptionData& captionData) override;
    void addListener(std::shared_ptr<CaptionFrameParseListenerInterface> parseListener) override;
    void releaseResourcesFor(CaptionFrame::MediaPlayerSourceId captionId) override;
    ///@}

private:
    /**
     * Constructor.
     */
    LibwebvttParserAdapter(){};

    /**
     * Copy constructor.
     */
    LibwebvttParserAdapter(LibwebvttParserAdapter const&) = delete;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_LIBWEBVTTPARSERADAPTER_H_
