/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYERINTERFACE_H_

#include <chrono>
#include <memory>

#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

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
    virtual void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) = 0;

    /**
     * This function removes an observer from @c AudioPlayer so that it will no longer be notified of
     * AudioPlayer state changes.
     *
     * @param observer The @c AudioPlayerObserverInterface
     */
    virtual void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer) = 0;

    /**
     * This function retrieves the offset of the current AudioItem the @c AudioPlayer is handling.
     * @note This function is blocking.
     *
     * @return This returns the offset in millisecond.
     */
    virtual std::chrono::milliseconds getAudioItemOffset() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYERINTERFACE_H_
