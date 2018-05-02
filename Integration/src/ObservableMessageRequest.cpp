/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Integration/ObservableMessageRequest.h"

#include <iostream>

namespace alexaClientSDK {
namespace integration {

/// String to identify log entries originating from this file.
static const std::string TAG("ObservableMessageRequest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;

/// The field name for the user voice attachment.
static const std::string AUDIO_ATTACHMENT_FIELD_NAME = "audio";

ObservableMessageRequest::ObservableMessageRequest(
    const std::string& jsonContent,
    std::shared_ptr<AttachmentReader> attachmentReader) :
        MessageRequest{jsonContent},
        m_sendMessageStatus(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PENDING),
        m_sendCompleted{false},
        m_exceptionReceived{false} {
    if (attachmentReader) {
        addAttachmentReader(AUDIO_ATTACHMENT_FIELD_NAME, attachmentReader);
    }
}

void ObservableMessageRequest::sendCompleted(
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("onSendCompleted").d("status", sendMessageStatus));
    m_sendMessageStatus = sendMessageStatus;
    m_sendCompleted = true;
    m_wakeTrigger.notify_all();
}

avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status ObservableMessageRequest::getSendMessageStatus()
    const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sendMessageStatus;
}

bool ObservableMessageRequest::waitFor(
    const avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus,
    const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_wakeTrigger.wait_for(
        lock, duration, [this, sendMessageStatus]() { return sendMessageStatus == m_sendMessageStatus; });
}

void ObservableMessageRequest::exceptionReceived(const std::string& exceptionMessage) {
    ACSDK_DEBUG(LX("onExceptionReceived").d("status", exceptionMessage));
    m_exceptionReceived = true;
}

bool ObservableMessageRequest::hasSendCompleted() {
    return m_sendCompleted;
}

bool ObservableMessageRequest::wasExceptionReceived() {
    return m_exceptionReceived;
}

}  // namespace integration
}  // namespace alexaClientSDK
