/*
 * MessageRequest.cpp
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

#include "AVSCommon/AVS/MessageRequest.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

MessageRequest::MessageRequest(const std::string & jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) :
        m_jsonContent{jsonContent}, m_attachmentReader{attachmentReader} {
}

MessageRequest::~MessageRequest() {

}

std::string MessageRequest::getJsonContent() {
    return m_jsonContent;
}

std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> MessageRequest::getAttachmentReader() {
    return m_attachmentReader;
}

void MessageRequest::onSendCompleted(Status status) {
    // default no-op
}

void MessageRequest::onExceptionReceived(const std::string & exceptionMessage) {
    // default no-op
}

std::string MessageRequest::statusToString(Status status) {
    switch (status) {
        case Status::PENDING:
            return "PENDING";
        case Status::SUCCESS:
            return "SUCCESS";
        case Status::NOT_CONNECTED:
            return "NOT_CONNECTED";
        case Status::NOT_SYNCHRONIZED:
            return "NOT_SYNCHRONIZED";
        case Status::TIMEDOUT:
            return "TIMEDOUT";
        case Status::PROTOCOL_ERROR:
            return "PROTOCOL_ERROR";
        case Status::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case Status::SERVER_INTERNAL_ERROR:
            return "SERVER_INTERNAL_ERROR";
        case Status::REFUSED:
            return "REFUSED";
        case Status::CANCELED:
            return "CANCELED";
        case Status::THROTTLED:
            return "THROTTLED";
        case Status::INVALID_AUTH:
            return "INVALID_AUTH";
    }

    return "sendMessageStatusToString_UNHANDLED_ERROR";
}

} // namespace avs
} // namespace avsCommon
} // namespace alexaClientSDK
