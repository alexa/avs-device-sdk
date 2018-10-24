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

#ifndef ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERUTILS_H_
#define ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERUTILS_H_

#include <AVSCommon/SDKInterfaces/Audio/EqualizerTypes.h>
#include <AVSCommon/Utils/Error/SuccessResult.h>

namespace alexaClientSDK {
namespace equalizer {

/**
 * Class containing Equalizer-related utility methods.
 */
class EqualizerUtils {
public:
    /**
     * Serializes @c EqualizerState into a string.
     *
     * @param state Equalizer state to serialize.
     * @return String containing serialized Equalizer state.
     */
    static std::string serializeEqualizerState(const avsCommon::sdkInterfaces::audio::EqualizerState& state);

    /**
     * Parses serialized Equalizer state back into @c EqualizerState object.
     *
     * @param serializedState String containing serialized state.
     * @return Result containing reconstructed @c EqualizerState object on success.
     */
    static avsCommon::utils::error::SuccessResult<avsCommon::sdkInterfaces::audio::EqualizerState>
    deserializeEqualizerState(const std::string& serializedState);
};

}  // namespace equalizer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_EQUALIZERUTILS_H_
