/*
 * PlaybackControllerInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKCONTROLLERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The PlaybackController Interface can be used to send events for navigating a playback queue with
 * an on-client button press or GUI affordance, rather than through a speech request.
 */
class PlaybackControllerInterface {
public:
    /**
     * Destructor
     */
    virtual ~PlaybackControllerInterface() = default;

    /**
     * This method can be called by the client when "Play" is pressed on a physical button or on the GUI.
     * A PlayCommandIssued event message will be sent to AVS.
     */
    virtual void playButtonPressed() = 0;

    /**
     * This method can be called by the client when "Pause" is pressed on a physical button or on the GUI.
     * A PauseCommandIssued event message will be sent to AVS.
     */
    virtual void pauseButtonPressed() = 0;

    /**
     * This method can be called by the client when "Next" is pressed on a physical button or on the GUI.
     * A NextCommandIssued event message will be sent to AVS.
     */
    virtual void nextButtonPressed() = 0;

    /**
     * This method can be called by the client when "Previous" is pressed on a physical button or on the GUI.
     * A PreviousCommandIssued event message will be sent to AVS.
     */
    virtual void previousButtonPressed() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_PLAYBACKCONTROLLERINTERFACE_H_
