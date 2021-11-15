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

#ifndef ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYERINTERFACE_H_

#include <chrono>
#include <memory>

#include "acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkAudioPlayerInterfaces {

/**
 * This class provides an interface to the @c AudioPlayer.
 */
class AudioPlayerInterface {
public:
    /**
     * Destructor
     */
    virtual ~AudioPlayerInterface() = default;

    /**
     * This function adds an observer to @c AudioPlayer so that it will get notified for AudioPlayer state changes.
     *
     * @param observer The @c AudioPlayerObserverInterface
     */
    virtual void addObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) = 0;

    /**
     * This function removes an observer from @c AudioPlayer so that it will no longer be notified of
     * AudioPlayer state changes.
     *
     * @param observer The @c AudioPlayerObserverInterface
     */
    virtual void removeObserver(std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface> observer) = 0;

    /**
     * This function stops playback of the current song.
     * @deprecated Use @c LocalPlayerControlInterface.playbackStop instead
     */
    virtual void stopPlayback() = 0;
};

}  // namespace acsdkAudioPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYERINTERFACES_INCLUDE_ACSDKAUDIOPLAYERINTERFACES_AUDIOPLAYERINTERFACE_H_
