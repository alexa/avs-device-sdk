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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTERINTERFACE_H_

#include "DelayInterface.h"

#include <Captions/CaptionFrame.h>

namespace alexaClientSDK {
namespace captions {

/**
 * An abstract class that is responsible for handling the timed display of @c CaptionFrame objects. Each @c
 * CaptionFrame's delay and duration values should be used with the @c DelayInterface to maintain this timing.
 *
 * Implementations of this class must be thread-safe and allow for asynchronous calls to any of the provided methods.
 */
class CaptionTimingAdapterInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CaptionTimingAdapterInterface() = default;

    /**
     * Enqueues a @c CaptionFrame object for display. This method must begin the serial presentation of the enqueued
     * caption frames if such presentation is not already taking place and @c autostart is true. The order of the
     * caption frames must be maintained as first-in-first-out.
     *
     * @param captionFrame The @c CaptionFrame object to be enqueued for display.
     * @param autostart Determines if presentation should begin immediately if it has not already; defaults to true.
     */
    virtual void queueForDisplay(const CaptionFrame& captionFrame, bool autostart = true) = 0;

    /**
     * Resets the state of this adapter, in preparation for new captions content.
     */
    virtual void reset() = 0;

    /**
     * Resumes the playback of captions content, starting from the caption frame following the last one shown.
     */
    virtual void start() = 0;

    /**
     * Stops the playback of captions, forgetting which caption frame was last shown.
     */
    virtual void stop() = 0;

    /**
     * Pauses the playback of captions, keeping track of the last caption frame shown.
     */
    virtual void pause() = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTERINTERFACE_H_
