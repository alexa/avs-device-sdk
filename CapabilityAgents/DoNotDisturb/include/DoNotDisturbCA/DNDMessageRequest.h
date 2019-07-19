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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDMESSAGEREQUEST_H_

#include <future>
#include <string>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

/**
 * Message request used to send AVS events for DND feature.
 */
class DNDMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * Constructor
     *
     * @param jsonContent JSON representation of the message to send.
     */
    DNDMessageRequest(const std::string& jsonContent);

    ~DNDMessageRequest() override;

    /// @name MessageRequest overridden methods
    /// @{
    void sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    /// @}

    /**
     * Returns a future to track completion state of the request.
     *
     * @return A future to track completion state of the request.
     */
    std::shared_future<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> getCompletionFuture();

private:
    /// Promise to host std::future for the request.
    std::promise<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> m_promise;

    /// Shared future to be exposed to waiters.
    std::shared_future<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> m_future;

    /// Flag indicating whether the request has been completed.
    bool m_isCompleted;
};

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDMESSAGEREQUEST_H_
