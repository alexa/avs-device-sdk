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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGEROBSERVERINTERFACE_H_

#include <memory>

#include "AVSCommon/AVS/CapabilityTag.h"
#include "AVSCommon/AVS/CapabilityState.h"
#include "AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface used to get notified of changes to the context of one or more endpoints.
 */
class ContextManagerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ContextManagerObserverInterface() = default;

    /**
     * Notifies the observer that a change of state has been proactively reported.
     *
     * @param identifier Identifies the source of the state change.
     * @param state The new state.
     * @param cause The reason for the state change.
     */
    virtual void onStateChanged(
        const avs::CapabilityTag& identifier,
        const avs::CapabilityState& state,
        AlexaStateChangeCauseType cause) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGEROBSERVERINTERFACE_H_
