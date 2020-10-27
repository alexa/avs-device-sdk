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

#ifndef ACSDKEQUALIZERINTERFACES_EQUALIZERMODECONTROLLERINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_EQUALIZERMODECONTROLLERINTERFACE_H_

#include "EqualizerTypes.h"

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {

/**
 * Interface to handle equalizer modes. It is up to device manufacturer to implement the mode behavior. For example
 * vendor may wish to apply:
 * - Equalizer preset
 * - Volume scaling
 * - Audio effects such as reverb or surround.
 */
class EqualizerModeControllerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EqualizerModeControllerInterface() = default;

    /**
     * Changes the current equalizer mode. Equalizer state listeners will be notified only if this method returns true.
     * It is safe to change equalizer band values from this function, but changing mode is not allowed.
     *
     * @param mode The @c EqualizerMode to set.
     * @return True if mode has been successfully set, false otherwise.
     */
    virtual bool setEqualizerMode(EqualizerMode mode) = 0;
};

}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_EQUALIZERMODECONTROLLERINTERFACE_H_
