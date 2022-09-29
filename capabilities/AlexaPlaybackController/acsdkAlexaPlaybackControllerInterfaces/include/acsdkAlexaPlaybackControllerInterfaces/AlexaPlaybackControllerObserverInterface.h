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

#ifndef ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLEROBSERVERINTERFACE_H_
#define ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLEROBSERVERINTERFACE_H_

#include "acsdkAlexaPlaybackControllerInterfaces/PlaybackState.h"

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackControllerInterfaces {
/**
 * This interface is used to observe changes to the playback state of a device that is caused by the
 * @c AlexaPlaybackControllerInterface.
 */
class AlexaPlaybackControllerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AlexaPlaybackControllerObserverInterface() = default;

    /**
     * Notifies the change in the playback state properties of the endpoint.
     *
     * @param playbackState The playback value specified using @c PlaybackState.
     */
    virtual void onPlaybackStateChanged(const acsdkAlexaPlaybackControllerInterfaces::PlaybackState& playbackState) = 0;
};

}  // namespace acsdkAlexaPlaybackControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLEROBSERVERINTERFACE_H_
