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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDERINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>

#include "AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * A @c PooledMediaResourceProviderInterface allows access to @c MediaPlayerInterface instances as needed (and if
 * available). This is a capability needed to support pre-buffering.
 *
 * It also provides access to the @c ChannelVolumeInterfaces associated with the media players managed by this provider
 * in order to perform volume adjustments on the players.
 *
 * Instances are not expected to be Thread-safe.
 */
class PooledMediaResourceProviderInterface : public MediaPlayerFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PooledMediaResourceProviderInterface() = default;

    /**
     * Gets all @c ChannelVolumeInterfaces associated with the media players managed by this instance.
     * @note The number of channel volume interfaces is not guaranteed to match the number of media players managed
     * by the instance.
     *
     * @return A vector of pointers to @c ChannelVolumeInterfaces.
     */
    virtual std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> getSpeakers() const = 0;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDERINTERFACE_H_
