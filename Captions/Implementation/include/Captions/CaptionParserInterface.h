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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONPARSERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONPARSERINTERFACE_H_

#include <memory>

#include <Captions/CaptionData.h>
#include <Captions/CaptionFrame.h>

#include "CaptionFrameParseListenerInterface.h"

namespace alexaClientSDK {
namespace captions {

/**
 * Provides a standardized interface for caption parsing implementations. Implementers of this interface handle requests
 * to parse raw captions content.
 */
class CaptionParserInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CaptionParserInterface() = default;

    /**
     * Start parsing the provided raw caption data. The @c captionId that is received here should be retained and passed
     * along with the parsed result to the @c CaptionFrameParseListenerInterface. This is to keep track of which
     * incoming @c CaptionData go with the outgoing parsed @c CaptionFrames.
     *
     * @param captionId The identifier for the incoming caption data.
     * @param captionData the raw, un-processed caption to parse.
     */
    virtual void parse(CaptionFrame::MediaPlayerSourceId captionId, const CaptionData& captionData) = 0;

    /**
     * Notify the parser that resources related to this caption ID are no longer needed and can be safely released. This
     * function might be a no-op, depending on the caption parser implementation, but is provided in case
     * static resources are maintained between parse requests, such as if callback functions are used to communicate
     * with the parser.
     *
     * @param captionId the ID corresponding with the resources that should be freed, should a value that was previously
     * received with the @c parse() function.
     */
    virtual void releaseResourcesFor(CaptionFrame::MediaPlayerSourceId captionId) = 0;

    /**
     * Gives the caption parsing implementation a handle to a listener so that it can be notified when @c CaptionData
     * objects have been parsed.
     *
     * @param parseListener the @c CaptionFrameParseListenerInterface instance which can receive the parsed objects.
     */
    virtual void addListener(std::shared_ptr<CaptionFrameParseListenerInterface> parseListener) = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONPARSERINTERFACE_H_
