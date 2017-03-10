/*
 * AVSMessageHeader.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVSMessageHeader.h"

namespace alexaClientSDK {
namespace avsCommon {

std::string AVSMessageHeader::getNamespace() const {
    return m_namespace;
}

std::string AVSMessageHeader::getName() const {
    return m_name;
}

std::string AVSMessageHeader::getMessageId() const {
    return m_messageId;
}

std::string AVSMessageHeader::getDialogRequestId() const {
    return m_dialogRequestId;
}

} // namespace avsCommon
} // namespace alexaClientSDK