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

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/MessageRequestQueue.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
#define TAG "MessageRequestQueue"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MessageRequestQueue::MessageRequestQueue() : m_isWaitingForAcknowledgement{false} {
}

void MessageRequestQueue::enqueueRequest(std::shared_ptr<MessageRequest> messageRequest) {
    if (messageRequest != nullptr) {
        m_queue.push_back({std::chrono::steady_clock::now(), messageRequest});
    } else {
        ACSDK_ERROR(LX("enqueueRequest").d("reason", "nullMessageRequest"));
    }
}

avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> MessageRequestQueue::peekRequestTime() {
    if (!m_queue.empty()) {
        return m_queue.front().first;
    }

    return avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>>();
}

std::shared_ptr<MessageRequest> MessageRequestQueue::dequeueOldestRequest() {
    if (m_queue.empty()) {
        return nullptr;
    }

    auto result = m_queue.front().second;
    m_queue.pop_front();
    return result;
}

std::shared_ptr<avsCommon::avs::MessageRequest> MessageRequestQueue::dequeueSendableRequest() {
    for (auto it = m_queue.begin(); it != m_queue.end(); it++) {
        if (!m_isWaitingForAcknowledgement || !it->second->getIsSerialized()) {
            auto result = it->second;
            m_queue.erase(it);
            return result;
        }
    }
    return nullptr;
}

bool MessageRequestQueue::isMessageRequestAvailable() const {
    for (auto it = m_queue.begin(); it != m_queue.end(); it++) {
        if (!m_isWaitingForAcknowledgement || !it->second->getIsSerialized()) {
            return true;
        }
    }
    return false;
}

void MessageRequestQueue::setWaitingForSendAcknowledgement() {
    m_isWaitingForAcknowledgement = true;
}

void MessageRequestQueue::clearWaitingForSendAcknowledgement() {
    m_isWaitingForAcknowledgement = false;
}

bool MessageRequestQueue::empty() const {
    return m_queue.empty();
}

void MessageRequestQueue::clear() {
    m_queue.clear();
}

}  // namespace acl
}  // namespace alexaClientSDK
