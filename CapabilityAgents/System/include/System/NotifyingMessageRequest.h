/*
 * NotifyingMessageRequest.h
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_NOTIFYING_MESSAGE_REQUEST_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_NOTIFYING_MESSAGE_REQUEST_H_

#include <AVSCommon/AVS/MessageRequest.h>

#include "StateSynchronizer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implements @c MessageRequests to alert observers upon completion of the message.
 */
class NotifyingMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * @copyDoc avsCommon::avs::MessageRequest()
     *
     * Construct a @c MessageRequest while binding it to a @c StateSynchronizer.
     *
     * @param callback The function to be called when @c onSendCompleted is invoked.
     */
    NotifyingMessageRequest(const std::string& jsonContent, std::shared_ptr<StateSynchronizer> stateSynchronizer);

    /// @name MessageRequest functions.
    /// @{
    void onSendCompleted(avsCommon::avs::MessageRequest::Status status) override;
    /// @}
    
private:
    /// The @c StateSynchronizer to be notified when @c onSendCompleted is called.
    std::shared_ptr<StateSynchronizer> m_stateSynchronizer;
};

} // namespace system
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_NOTIFYING_MESSAGE_REQUEST_H_
