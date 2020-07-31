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

#include "ACL/Transport/SynchronizedMessageRequestQueue.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("SynchronizedMessageRequestQueue");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

SynchronizedMessageRequestQueue::~SynchronizedMessageRequestQueue() {
    clearWaitingForSendAcknowledgement();
    clear();
}

void SynchronizedMessageRequestQueue::enqueueRequest(std::shared_ptr<MessageRequest> messageRequest) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_requestQueue.enqueueRequest(std::move(messageRequest));
}

avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> SynchronizedMessageRequestQueue::
    peekRequestTime() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_requestQueue.peekRequestTime();
}

std::shared_ptr<MessageRequest> SynchronizedMessageRequestQueue::dequeueOldestRequest() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_requestQueue.dequeueOldestRequest();
}

std::shared_ptr<MessageRequest> SynchronizedMessageRequestQueue::dequeueSendableRequest() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_requestQueue.dequeueSendableRequest();
}

bool SynchronizedMessageRequestQueue::isMessageRequestAvailable() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_requestQueue.isMessageRequestAvailable();
}

void SynchronizedMessageRequestQueue::setWaitingForSendAcknowledgement() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_requestQueue.setWaitingForSendAcknowledgement();
}

void SynchronizedMessageRequestQueue::clearWaitingForSendAcknowledgement() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_requestQueue.clearWaitingForSendAcknowledgement();
}

bool SynchronizedMessageRequestQueue::empty() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_requestQueue.empty();
}

void SynchronizedMessageRequestQueue::clear() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_requestQueue.clear();
}

}  // namespace acl
}  // namespace alexaClientSDK
