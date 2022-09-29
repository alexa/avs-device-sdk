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

#include "AVSCommon/AVS/EditableMessageRequest.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "EditableMessageRequest"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

EditableMessageRequest::EditableMessageRequest(const MessageRequest& messageRequest) :
        avsCommon::avs::MessageRequest(messageRequest) {
}

void EditableMessageRequest::setJsonContent(const std::string& json) {
    m_jsonContent = json;
}

void EditableMessageRequest::setAttachmentReaders(const std::vector<std::shared_ptr<NamedReader>>& attachmentReaders) {
    m_readers.clear();
    for (auto& it : attachmentReaders) {
        if (it) {
            addAttachmentReader(it->name, it->reader);
        } else {
            ACSDK_ERROR(LX("Failed to set attachment readers").d("reason", "nullAttachment"));
        }
    }
}

void EditableMessageRequest::setMessageRequestResolveFunction(
    const MessageRequest::MessageRequestResolveFunction& resolver) {
    m_resolver = resolver;
}
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
