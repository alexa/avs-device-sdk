/*
* Message.cpp
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

#include "AVSCommon/AVS/Message.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

Message::Message(const std::string& json, std::shared_ptr<std::istream> binaryContent)
        : m_jsonContent{json}, m_binaryContent{binaryContent} {
}

Message::Message(const std::string& json, std::shared_ptr<avsCommon::AttachmentManagerInterface> attachmentManager)
        : m_jsonContent{json}, m_attachmentManager{attachmentManager} {
}

std::string Message::getJSONContent() const {
    return m_jsonContent;
}

std::shared_ptr<std::istream> Message::getAttachment() {
    return m_binaryContent;
}

std::shared_ptr<avsCommon::AttachmentManagerInterface> Message::getAttachmentManager() const {
    return m_attachmentManager;
}

} // namespace avs
} // namespace avsCommon
} // namespace alexaClientSDK
