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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDERINTERNALINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDERINTERNALINTERFACE_H_

#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

/**
 * Specialization of AlexaInterfaceMessageSenderInterface to allow sending of internal AlexaInterface messages.
 */
class AlexaInterfaceMessageSenderInternalInterface
        : public avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AlexaInterfaceMessageSenderInternalInterface() = default;

    /**
     * Send an Alexa.StateReport event.  Since these events require context, the event will be enqueued and this method
     * will return immediately.  The event will be sent once context has been received from ContextManager.
     *
     * @param instance The instance ID of the responding capability.
     * @param correlationToken The correlation token from the directive to which we are responding.
     * @param endpoint The @c AVSMessageEndpoint to identify the endpoint related to this event.
     * @return true if the event was successfuly enqueued, false on failure.
     */
    virtual bool sendStateReportEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint) = 0;
};

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDERINTERNALINTERFACE_H_
