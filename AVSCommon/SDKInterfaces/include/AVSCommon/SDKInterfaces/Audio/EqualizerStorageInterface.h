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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACE_H_

#include <memory>

#include <AVSCommon/Utils/Error/SuccessResult.h>

#include "EqualizerTypes.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * Interface to persist the last set to @c EqualizerController.
 */
class EqualizerStorageInterface {
public:
    /**
     *  Destructor.
     */
    virtual ~EqualizerStorageInterface() = default;

    /**
     * Saves equalizer state to the storage.
     *
     * @param state @c EqualizerState to store.
     */
    virtual void saveState(const EqualizerState& state) = 0;

    /**
     * Loads a stored equalizer state from the storage.
     *
     * @return State retrieved from the storage or @c nullptr if no state is stored.
     */
    virtual avsCommon::utils::error::SuccessResult<EqualizerState> loadState() = 0;

    /**
     * Clears all the EQ settings from the storage. The next call to @c loadState() must return no state unless another
     * state is saved between @c clear() and @c loadState().
     */
    virtual void clear() = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACE_H_
