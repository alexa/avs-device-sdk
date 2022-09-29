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
#ifndef ACSDK_ALEXAPRESENTATIONINTERFACES_ALEXAPRESENTATIONCAPABILITYAGENTINTERFACE_H_
#define ACSDK_ALEXAPRESENTATIONINTERFACES_ALEXAPRESENTATIONCAPABILITYAGENTINTERFACE_H_

#include <chrono>
#include <memory>
#include <string>

#include <acsdk/APLCapabilityCommonInterfaces/PresentationToken.h>

namespace alexaClientSDK {
namespace alexaPresentationInterfaces {
/**
 * Defines a Contract for clients to communicate with the AlexaPresentation Capability agent.
 * Clients will use this interface to send events to AVS as defined in
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation.html#events
 */
class AlexaPresentationCapabilityAgentInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AlexaPresentationCapabilityAgentInterface() = default;

    /**
     * This function notifies the Capability Agent that the Presentation with the given token has been dismissed
     *
     * @param token The token for the presentation which was dismissed
     */
    virtual void onPresentationDismissed(const aplCapabilityCommonInterfaces::PresentationToken& token) = 0;
};
}  // namespace alexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONINTERFACES_ALEXAPRESENTATIONCAPABILITYAGENTINTERFACE_H_