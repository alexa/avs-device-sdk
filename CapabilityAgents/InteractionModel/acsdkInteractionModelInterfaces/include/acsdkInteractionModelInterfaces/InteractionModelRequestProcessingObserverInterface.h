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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODELINTERFACES_INCLUDE_ACSDKINTERACTIONMODELINTERFACES_INTERACTIONMODELREQUESTPROCESSINGOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODELINTERFACES_INCLUDE_ACSDKINTERACTIONMODELINTERFACES_INTERACTIONMODELREQUESTPROCESSINGOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkInteractionModelInterfaces {

/// A directive observer for an @c InteractionModelCapabilityAgent.
class InteractionModelRequestProcessingObserverInterface {
public:
    /// Destructor.
    virtual ~InteractionModelRequestProcessingObserverInterface() = default;

    /**
     * Used to notify the observer that a Request Processing Started was received.
     */
    virtual void onRequestProcessingStarted() = 0;

    /**
     * Used to notify the observer that a Request Processing Completed was received.
     */
    virtual void onRequestProcessingCompleted() = 0;
};

}  // namespace acsdkInteractionModelInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODELINTERFACES_INCLUDE_ACSDKINTERACTIONMODELINTERFACES_INTERACTIONMODELREQUESTPROCESSINGOBSERVERINTERFACE_H_
