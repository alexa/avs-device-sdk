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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALPLAYBACKHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALPLAYBACKHANDLERINTERFACE_H_

#include <AVSCommon/AVS/PlaybackButtons.h>

#include <chrono>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows a local UI to request playback actions using local control
 */
class LocalPlaybackHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~LocalPlaybackHandlerInterface() = default;

    /// Available local operations.
    enum PlaybackOperation { STOP_PLAYBACK, PAUSE_PLAYBACK, RESUME_PLAYBACK };

    /**
     * Request the handler to perform a local playback operation.
     *
     * @param op Operation to request
     * @return true if successful, false if the operation cannot be performed locally.
     */
    virtual bool localOperation(PlaybackOperation op) = 0;

    /**
     * Request the handler to perform a local seek operation.
     *
     * @param location Position to seek to
     * @param fromStart true to seek to absolute location, false to seek reletive to current location.
     * @return true if successful, false if the operation cannot be performed locally.
     */
    virtual bool localSeekTo(std::chrono::milliseconds location, bool fromStart) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALPLAYBACKHANDLERINTERFACE_H_
