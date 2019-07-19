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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_

#include <chrono>
#include <string>

#include <AVSCommon/AVS/PlayerActivity.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows any observers of the @c AudioPlayer to be notified of changes in the @c AudioPlayer audio state.
 */
class AudioPlayerObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~AudioPlayerObserverInterface() = default;

    /// The context of the AudioPlayer when the observer is notified of the @c PlayerActivity state change.
    struct Context {
        /// The ID of the @c AudioItem that the @ AudioPlayer is handling.
        std::string audioItemId;

        /// The offset in millisecond from the start of the @c AudioItem.
        std::chrono::milliseconds offset;
    };

    /**
     * Used to notify the observer when the @c AudioPlayer has a change in @c PlayerActivity.
     *
     * @param state The @c PlayerActivity of the @c AudioPlayer.
     * @param context The @c Context of the @c AudioPlayer.
     */
    virtual void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOPLAYEROBSERVERINTERFACE_H_
