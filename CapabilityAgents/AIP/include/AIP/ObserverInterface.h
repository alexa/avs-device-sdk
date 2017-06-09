/**
 * ObserverInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_OBSERVER_INTERFACE_H_

#include "AudioInputProcessor.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

/// A state observer for an @c AudioInputProcessor.
class ObserverInterface {
public:
    /// Destructor.
    virtual ~ObserverInterface() = default;

    /**
     * This function is called when the state of the observed @c AudioInputProcessor changes.  This function will block
     * processing of audio inputs, so implementations should return quickly.
     *
     * @param state The new state of the @c AudioInputProcessor.
     */
    virtual void onStateChanged(AudioInputProcessor::State state) = 0;
};

} // namespace aip
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_AIP_INCLUDE_AIP_OBSERVER_INTERFACE_H_
