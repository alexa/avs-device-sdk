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

#ifndef ACSDKEQUALIZERINTERFACES_EQUALIZERCONTROLLERLISTENERINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_EQUALIZERCONTROLLERLISTENERINTERFACE_H_

#include "EqualizerTypes.h"

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {

/**
 * An interface to listen for @c EqualizerController state changes.
 */
class EqualizerControllerListenerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EqualizerControllerListenerInterface() = default;

    /**
     * Receives the new state of the @c EqualizerController. This callback is called after all changes has been applied.
     *
     * @param newState New state of the @c EqualizerController.
     */
    virtual void onEqualizerStateChanged(const EqualizerState& newState) = 0;

    /**
     * Receives the same state of the @c EqualizerController when equalizer setting is changed but to an identical state
     * to the current state. This callback is called after all changes has been applied.
     *
     * @param newState New state of the @c EqualizerController.
     */
    virtual void onEqualizerSameStateChanged(const EqualizerState& newState) = 0;
};

}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_EQUALIZERCONTROLLERLISTENERINTERFACE_H_
