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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERINTERFACE_H_

#include "EqualizerTypes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * Interface performing the actual equalization of the audio.
 */
class EqualizerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EqualizerInterface() = default;

    /**
     * Changes the equalization parameters for the audio.
     *
     * @param bandLevelMap EQ bands and their levels to be applied. Levels for all bands supported must be provided.
     */
    virtual void setEqualizerBandLevels(EqualizerBandLevelMap bandLevelMap) = 0;

    /**
     * Returns the minimum band level (dB) supported by this equalizer.
     *
     * @return The minimum band level (dB) supported by this equalizer.
     */
    virtual int getMinimumBandLevel() = 0;

    /**
     * Returns the maximum band level (dB) supported by this equalizer.
     *
     * @return The maximum band level (dB) supported by this equalizer.
     */
    virtual int getMaximumBandLevel() = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERINTERFACE_H_
