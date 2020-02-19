/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
static const std::string TAG("MessageRequestQueue");

static const std::string EMPTY_QUEUE_NAME = "";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MessageRequestQueue::MessageRequestQueue() : m_size{0} {
}

MessageRequestQueue::~MessageRequestQueue() {
    clearWaitingFlagForQueue();
    clear();
}

void MessageRequestQueue::enqueueRequest(std::shared_ptr<MessageRequest> messageRequest) {
    if (messageRequest != nullptr) {
        m_sendQueue.queue.push_back({std::chrono::steady_clock::now(), messageRequest});
        m_size++;
    } else {
        ACSDK_ERROR(LX("enqueueRequest").d("reason", "nullMessageRequest"));
    }
}

avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> MessageRequestQueue::peekRequestTime() {
    if (m_size > 0) {
        return m_sendQueue.queue.front().first;
    }

    return avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>>();
}

std::shared_ptr<MessageRequest> MessageRequestQueue::dequeueRequest() {
    std::shared_ptr<MessageRequest> messageRequest;

    if (m_size > 0) {
        messageRequest = m_sendQueue.queue.front().second;
        m_sendQueue.queue.pop_front();
        m_size--;
    }
    return messageRequest;
}

bool MessageRequestQueue::isMessageRequestAvailable() const {
    return !m_sendQueue.queue.empty() && !m_sendQueue.isQueueWaitingForResponse;
}

void MessageRequestQueue::setWaitingFlagForQueue() {
    m_sendQueue.isQueueWaitingForResponse = true;
}

void MessageRequestQueue::clearWaitingFlagForQueue() {
    m_sendQueue.isQueueWaitingForResponse = false;
}

bool MessageRequestQueue::empty() const {
    return (0 == m_size);
}

void MessageRequestQueue::clear() {
    m_sendQueue.queue.clear();
    m_size = 0;
}

}  // namespace acl
}  // namespace alexaClientSDK
