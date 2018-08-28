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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKHANDLERINTERFACE_H_

#include <AVSCommon/AVS/PlaybackButtons.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows a playback button press handler
 * to be notified a playback button is pressed.
 */
class PlaybackHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PlaybackHandlerInterface() = default;

    /**
     * Used to notify the handler when a playback button is pressed.
     *
     * @param button The button that has been pressed.
     */
    virtual void onButtonPressed(alexaClientSDK::avsCommon::avs::PlaybackButton button) = 0;

    /**
     * Used to notify the handler when a playback toggle is pressed.
     *
     * @param toggle The toggle that has been pressed.
     * @param action The boolean action for the toggle state
     */
    virtual void onTogglePressed(alexaClientSDK::avsCommon::avs::PlaybackToggle toggle, bool action) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKHANDLERINTERFACE_H_
