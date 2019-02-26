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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "DoNotDisturbCA/DNDMessageRequest.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("DNDMessageRequest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DNDMessageRequest::DNDMessageRequest(const std::string& jsonContent) :
        MessageRequest(jsonContent, ""),
        m_isCompleted{false} {
    m_future = m_promise.get_future();
}

void DNDMessageRequest::sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    MessageRequest::sendCompleted(status);
    if (!m_isCompleted) {
        ACSDK_DEBUG9(LX(__func__).d("Completed with status", status));
        m_promise.set_value(status);
    } else {
        ACSDK_ERROR(LX("sendCompletedFailed").d("reason", "sendCompleted must be called only once."));
    }
    m_isCompleted = true;
}

std::shared_future<MessageRequestObserverInterface::Status> DNDMessageRequest::getCompletionFuture() {
    return m_future;
}

DNDMessageRequest::~DNDMessageRequest() {
    if (!m_isCompleted) {
        ACSDK_WARN(LX(__func__).m("Destroying while message delivery has not been completed yet."));
        m_promise.set_value(MessageRequestObserverInterface::Status::CANCELED);
    }
}

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
