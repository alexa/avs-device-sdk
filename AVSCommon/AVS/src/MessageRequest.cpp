/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/MessageRequest.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("MessageRequest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MessageRequest::MessageRequest(const std::string& jsonContent, const std::string& uriPathExtension) :
        m_jsonContent{jsonContent},
        m_uriPathExtension{uriPathExtension} {
}

MessageRequest::~MessageRequest() {
}

void MessageRequest::addAttachmentReader(
    const std::string& name,
    std::shared_ptr<attachment::AttachmentReader> attachmentReader) {
    if (!attachmentReader) {
        ACSDK_ERROR(LX("addAttachmentReaderFailed").d("reason", "nullAttachment"));
        return;
    }

    auto namedReader = std::make_shared<MessageRequest::NamedReader>(name, attachmentReader);
    m_readers.push_back(namedReader);
}

std::string MessageRequest::getJsonContent() {
    return m_jsonContent;
}

std::string MessageRequest::getUriPathExtension() {
    return m_uriPathExtension;
}

int MessageRequest::attachmentReadersCount() {
    return m_readers.size();
}

std::shared_ptr<MessageRequest::NamedReader> MessageRequest::getAttachmentReader(size_t index) {
    if (m_readers.size() <= index) {
        ACSDK_ERROR(LX("getAttachmentReaderFailed").d("reason", "index out of bound").d("index", index));
        return nullptr;
    }

    return m_readers[index];
}

void MessageRequest::sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onSendCompleted(status);
    }
}

void MessageRequest::exceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("onExceptionReceived").d("exception", exceptionMessage));

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onExceptionReceived(exceptionMessage);
    }
}

void MessageRequest::addObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(observer);
}

void MessageRequest::removeObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(observer);
}

using namespace avsCommon::sdkInterfaces;

bool MessageRequest::isServerStatus(MessageRequestObserverInterface::Status status) {
    switch (status) {
        case MessageRequestObserverInterface::Status::SUCCESS:
        case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
        case MessageRequestObserverInterface::Status::CANCELED:
        case MessageRequestObserverInterface::Status::THROTTLED:
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
        case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
            return true;
        case MessageRequestObserverInterface::Status::PENDING:
        case MessageRequestObserverInterface::Status::NOT_CONNECTED:
        case MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED:
        case MessageRequestObserverInterface::Status::TIMEDOUT:
        case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
        case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
        case MessageRequestObserverInterface::Status::REFUSED:
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
            return false;
    }

    return false;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
