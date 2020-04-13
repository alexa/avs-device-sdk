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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * An Observer interface to provide notification of @c MediaPlayer availability.
 */
class MediaPlayerFactoryObserverInterface {
public:
    virtual ~MediaPlayerFactoryObserverInterface() = default;

    /**
     * Called when an instance of a @c MediaPlayerInterface is available.
     * This may be called synchronously with @c MediaPlayerFactoryInterface::releaseMediaPlayer()
     */
    virtual void onReadyToProvideNextPlayer() = 0;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYOBSERVERINTERFACE_H_
