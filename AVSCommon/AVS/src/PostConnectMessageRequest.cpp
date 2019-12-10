/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/PostConnectMessageRequest.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectMessageRequest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

PostConnectMessageRequest::PostConnectMessageRequest(const std::string& jsonContent) :
        MessageRequest{jsonContent},
        m_sendMessageStatus{MessageRequestObserverInterface::Status::TIMEDOUT},
        m_responseReceived{false},
        m_isRequestShuttingDown{false} {
}

void PostConnectMessageRequest::sendCompleted(
    alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus) {
    MessageRequest::sendCompleted(sendMessageStatus);
    std::lock_guard<std::mutex> lock{m_requestMutex};
    if (!m_responseReceived) {
        m_sendMessageStatus = sendMessageStatus;
        m_responseReceived = true;
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "sendCompletedCalled when m_responseReceived"));
    }
    m_requestCv.notify_one();
}

MessageRequestObserverInterface::Status PostConnectMessageRequest::waitForCompletion() {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    /// MessageRequests timeout after 30 seconds.
    m_requestCv.wait(lock, [this] { return m_isRequestShuttingDown || m_responseReceived; });
    return m_sendMessageStatus;
}

void PostConnectMessageRequest::shutdown() {
    std::lock_guard<std::mutex> lock{m_requestMutex};
    m_isRequestShuttingDown = true;
    m_sendMessageStatus = MessageRequestObserverInterface::Status::CANCELED;
    m_requestCv.notify_one();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
