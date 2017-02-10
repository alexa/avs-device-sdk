/*
 * ObservableMessageRequest.cpp
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

#include "Integration/ObservableMessageRequest.h"

namespace alexaClientSDK {
namespace integration {

ObservableMessageRequest::ObservableMessageRequest(std::shared_ptr<Message> message):
        MessageRequest(message),
        m_sendMessageStatus(SendMessageStatus::PENDING) {
}

void ObservableMessageRequest::onSendCompleted(SendMessageStatus sendMessageStatus) {
    m_sendMessageStatus = sendMessageStatus;
    m_wakeTrigger.notify_all();
}

SendMessageStatus ObservableMessageRequest::getSendMessageStatus() const {
    return m_sendMessageStatus;
}

bool ObservableMessageRequest::waitFor(const SendMessageStatus sendMessageStatus, const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_wakeTrigger.wait_for(lock, duration, [this, sendMessageStatus]() {
        return m_sendMessageStatus == sendMessageStatus;
    });
}

} // namespace integration
} // namespace alexaClientSDK
