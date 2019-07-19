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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERCONFIGURATIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERCONFIGURATIONINTERFACE_H_

#include <set>
#include <string>

#include <AVSCommon/Utils/Error/SuccessResult.h>
#include <AVSCommon/Utils/functional/hash.h>

#include "EqualizerTypes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * Interface to provide a configuration of equalizer capabilities supported by the device.
 */
class EqualizerConfigurationInterface {
public:
    /**
     *  Destructor.
     */
    virtual ~EqualizerConfigurationInterface() = default;

    /**
     * Returns true if equalizer is enabled, false otherwise.
     * @return True if equalizer is enabled, false otherwise.
     */
    virtual bool isEnabled() const = 0;

    /**
     * Returns a set of EQ bands supported by the device.
     *
     * @return A set of EQ bands supported by the device.
     */
    virtual std::set<EqualizerBand> getSupportedBands() const = 0;

    /**
     * Returns a set of EQ modes supported by the device.
     *
     * @return A set of EQ modes supported by the device.
     */
    virtual std::set<EqualizerMode> getSupportedModes() const = 0;

    /**
     * Returns the minimum band value supported by the device.
     *
     * @return The minimum band value supported by the device.
     */
    virtual int getMinBandLevel() const = 0;

    /**
     * Returns the maximum band value supported by the device.
     *
     * @return The maximum band value supported by the device.
     */
    virtual int getMaxBandLevel() const = 0;

    /**
     * Returns @c EqualizerState object defining default values for equalizer mode and band levels. These values should
     * be used when resetting any band to its default level.
     *
     * @return @c EqualizerState object representing the default state.
     */
    virtual EqualizerState getDefaultState() const = 0;

    /**
     * Checks if band is supported by the device.
     *
     * @param band @c EqualizerBand to check.
     * @return True if band is supported, false otherwise.
     */
    virtual bool isBandSupported(EqualizerBand band) const = 0;

    /**
     * Checks if mode is supported by the device.
     *
     * @param mode @c EqualizerMode to check.
     * @return True if mode is supported, false otherwise.
     */
    virtual bool isModeSupported(EqualizerMode mode) const = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERCONFIGURATIONINTERFACE_H_
