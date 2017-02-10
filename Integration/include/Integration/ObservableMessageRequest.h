/*
 * ObservableMessageRequest.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "ACL/MessageRequest.h"

namespace alexaClientSDK {
namespace integration {

using namespace alexaClientSDK::acl;

class ObservableMessageRequest : public MessageRequest {
public:
    ObservableMessageRequest(std::shared_ptr<Message>);
    void onSendCompleted(SendMessageStatus) override;
    SendMessageStatus getSendMessageStatus() const;
    bool waitFor(const SendMessageStatus, const std::chrono::seconds = std::chrono::seconds(10));
private:
    SendMessageStatus m_sendMessageStatus;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
};

} // namespace integration
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_
