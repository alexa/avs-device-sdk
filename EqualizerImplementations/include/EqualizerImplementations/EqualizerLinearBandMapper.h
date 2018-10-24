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

#ifndef ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERLINEARBANDMAPPER_H_
#define ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERLINEARBANDMAPPER_H_

#include <functional>
#include <memory>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerTypes.h>

#include "EqualizerBandMapperInterface.h"

namespace alexaClientSDK {
namespace equalizer {

/**
 * Provides linear mapping from AVS provided bands to target number of bands. Target bands are assumed to be equally
 * distributed among whole spectrum of equalization frequencies starting from basses to treble. I.e. if there are 10
 * target bands, band with index 0 would be the lowest frequency while band with index 9 will be the highest frequency.
 * AVS bands are also assumed to be distributed equally among the whole spectrum. I.e. if TREBLE is the only one band
 * provided, it will cover top third of target frequencies. All other bands will be assumed to have have equalization
 * level equal to 0 dB, meaning no equalization. This value will be distributed among target bands for lower two thirds.
 */
class EqualizerLinearBandMapper : public EqualizerBandMapperInterface {
public:
    /**
     * Factory method that creates a linear band mapper given the number of target bands.
     *
     * @param numberOfTargetBands
     * @return
     */
    static std::shared_ptr<EqualizerLinearBandMapper> create(int numberOfTargetBands);

    /// @name EqualizerBandMapperInterface methods
    /// @{
    void mapEqualizerBands(
        const avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap& bandLevelMap,
        std::function<void(int, int)> setBandCallback) override;
    ///}@

private:
    /**
     * Constructor that creates a linear band mapper given the number of target bands.
     *
     * @param numberOfTargetBands
     */
    explicit EqualizerLinearBandMapper(int numberOfTargetBands);

    /// Number of bands to map to.
    int m_numberOfTargetBands;
};

}  // namespace equalizer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERLINEARBANDMAPPER_H_
