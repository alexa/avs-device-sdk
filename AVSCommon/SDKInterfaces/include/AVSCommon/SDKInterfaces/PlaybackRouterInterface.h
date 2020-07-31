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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKROUTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKROUTERINTERFACE_H_

#include <memory>
#include <iostream>

#include "PlaybackHandlerInterface.h"
#include "LocalPlaybackHandlerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The @c PlaybackRouterInterface used to get a playback button press and
 * send it it to the current handler.
 */
class PlaybackRouterInterface {
public:
    /**
     * Destructor
     */
    virtual ~PlaybackRouterInterface() = default;

    /**
     * This method can be called by the client when a Button is pressed on a physical button or on the GUI.
     * A ButtonCommandIssued event message will be sent to the observer.
     * NOTE: Instances may attempt to handle button locally first.
     *
     * @param button The PlaybackButton type being pressed
     */
    virtual void buttonPressed(avsCommon::avs::PlaybackButton button) = 0;

    /**
     * This method can be called by the client when a Toggle is pressed on a physical button or on the GUI.
     * A ToggleCommandIssued event message will be sent to the observer.
     *
     * @param toggle The PlaybackToggle type being pressed
     * @param action The boolean action for the toggle state
     */
    virtual void togglePressed(avsCommon::avs::PlaybackToggle toggle, bool action) = 0;

    /**
     * This method sets the playback button press handler that any time a button is pressed
     * this handler will be called.
     *
     * @param handler - The handler to call on future playback button presses.
     * @param localHandler The handler to use for local requests.  nullptr to disable local requests
     */
    virtual void setHandler(
        std::shared_ptr<PlaybackHandlerInterface> handler,
        std::shared_ptr<LocalPlaybackHandlerInterface> localHandler = nullptr) = 0;

    /**
     * This method switches playback button pressess handling to the default handler.
     */
    virtual void switchToDefaultHandler() = 0;

    /**
     * This method switches playback button pressess handling to the default handler,
     * and enables the given local Handler.
     */
    virtual void useDefaultHandlerWith(std::shared_ptr<LocalPlaybackHandlerInterface> localHandler){};

    /**
     * Request the handler to perform a local playback operation.
     * NOTE: If local operation is not possable, Instances may fall back on @c ButtonPressed
     *
     * @param op Operation to request
     * @return true if successful, false if the operation cannot be performed locally.
     */
    virtual bool localOperation(LocalPlaybackHandlerInterface::PlaybackOperation op) {
        return false;
    };

    /**
     * Request the handler to perform a local seek operation.
     *
     * @param location Position to seek to
     * @param fromStart true to seek to absolute location, false to seek reletive to current location.
     * @return true if successful, false if the operation cannot be performed locally.
     */
    virtual bool localSeekTo(std::chrono::milliseconds location, bool fromStart) {
        return false;
    };
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKROUTERINTERFACE_H_
