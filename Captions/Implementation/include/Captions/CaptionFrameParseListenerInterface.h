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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONFRAMEPARSELISTENERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONFRAMEPARSELISTENERINTERFACE_H_

#include <Captions/CaptionFrame.h>

namespace alexaClientSDK {
namespace captions {

/**
 * An interface to notify when @c CaptionFrame objects have been parsed.
 */
class CaptionFrameParseListenerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CaptionFrameParseListenerInterface() = default;

    /**
     * Called by the caption parser implementation for each caption frame that has been parsed.
     *
     * @param captionFrame The single frame of captions, containing everything needed to be displayed.
     */
    virtual void onParsed(const CaptionFrame& captionFrame) = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONFRAMEPARSELISTENERINTERFACE_H_
