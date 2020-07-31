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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MEDIAPROPERTIESINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MEDIAPROPERTIESINTERFACE_H_

#include <chrono>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class provides an interface to query the properties of a media from a player.
 */
class MediaPropertiesInterface {
public:
    /**
     * Destructor
     */
    virtual ~MediaPropertiesInterface() = default;

    /**
     * This function retrieves the offset of the current AudioItem the player is handling.
     *
     * @return This returns the offset in milliseconds.
     */
    virtual std::chrono::milliseconds getAudioItemOffset() = 0;

    /**
     * Returns the duration of the current AudioItem the player is handling.
     *
     * @return returns the duration.
     */
    virtual std::chrono::milliseconds getAudioItemDuration() {
        return std::chrono::milliseconds::zero();
    };
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MEDIAPROPERTIESINTERFACE_H_
