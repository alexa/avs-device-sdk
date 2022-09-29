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

#include <acsdk/SpeakerManager/private/DefaultChannelVolumeFactory.h>

namespace alexaClientSDK {
namespace speakerManager {

std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface> DefaultChannelVolumeFactory::
    createChannelVolumeInterface(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        std::function<int8_t(int8_t)> volumeCurve) {
    return ChannelVolumeManager::create(std::move(speaker), std::move(type), std::move(volumeCurve));
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
