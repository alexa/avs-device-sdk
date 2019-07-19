/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERBANDMAPPERINTERFACE_H_
#define ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERBANDMAPPERINTERFACE_H_

#include <functional>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerTypes.h>

namespace alexaClientSDK {
namespace equalizer {

class EqualizerBandMapperInterface {
public:
    virtual ~EqualizerBandMapperInterface() = default;

    /**
     * Maps AVS band levels to the target equalizer bands. Number of target bands and mapping method are implementation
     * specific. All supported bands should be defined. It is up to the implementation to decide what levels
     * to use for missing bands.
     *
     * @param bandLevelMap Map of AVS bands and their levels.
     * @param setBandCallback Callback receiving the target band index and the level to be applied.
     * It is up to the user to set up implementation properly to be able to map index to the band. No particular order
     * of indices is guaranteed. The signature of the callback is void callback(int bandIndex, int level);
     */
    virtual void mapEqualizerBands(
        const avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap& bandLevelMap,
        std::function<void(int, int)> setBandCallback) = 0;
};

}  // namespace equalizer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERBANDMAPPERINTERFACE_H_
