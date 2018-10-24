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

/// @file MimeUtils.cpp

#include "AVSCommon/Utils/Common/MimeUtils.h"

#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>

#include "AVSCommon/Utils/Common/Common.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;

/// The newline characters that MIME parsers expect.
static const std::string MIME_NEWLINE = "\r\n";
/// The double dashes which may occur before and after a boundary string.
static const std::string MIME_BOUNDARY_DASHES = "--";
/// The MIME text expected before a JSON part.
static const std::string MIME_JSON_PREFIX_STRING = "Content-Type: application/json; charset=UTF-8";
/// The MIME text expected before a binary data part.
static const std::string MIME_ATTACHMENT_PREFIX_STRING = "Content-Type: application/octet-stream";
/// The MIME prefix for a content id header.
static const std::string MIME_CONTENT_ID_PREFIX_STRING = "Content-ID: ";
/// Our default timeout when validating if a MIME part was received by another object.
static const std::chrono::seconds WAIT_FOR_DIRECTIVE_TIMEOUT_IN_SECONDS = std::chrono::seconds(1);

TestMimeJsonPart::TestMimeJsonPart(
    const std::string& boundaryString,
    int dataSize,
    std::shared_ptr<TestableMessageObserver> messageObserver) :
        m_message{createRandomAlphabetString(dataSize)},
        m_messageObserver{messageObserver} {
    m_mimeString = MIME_JSON_PREFIX_STRING + MIME_NEWLINE + MIME_NEWLINE + m_message + MIME_NEWLINE +
                   MIME_BOUNDARY_DASHES + boundaryString;
}

TestMimeJsonPart::TestMimeJsonPart(
    const std::string& mimeString,
    const std::string& message,
    std::shared_ptr<TestableMessageObserver> messageObserver) :
        m_message{message},
        m_messageObserver{messageObserver},
        m_mimeString{mimeString} {
}

std::string TestMimeJsonPart::getMimeString() const {
    return m_mimeString;
}

bool TestMimeJsonPart::validateMimeParsing() {
    return m_messageObserver->waitForDirective(m_message, WAIT_FOR_DIRECTIVE_TIMEOUT_IN_SECONDS);
}

TestMimeAttachmentPart::TestMimeAttachmentPart(
    const std::string& boundaryString,
    const std::string& contextId,
    const std::string contentId,
    int dataSize,
    std::shared_ptr<AttachmentManager> attachmentManager) :
        m_contextId{contextId},
        m_contentId{contentId},
        m_attachmentData{createRandomAlphabetString(dataSize)},
        m_attachmentManager{attachmentManager} {
    m_mimeString = MIME_CONTENT_ID_PREFIX_STRING + m_contentId + MIME_NEWLINE + MIME_ATTACHMENT_PREFIX_STRING +
                   MIME_NEWLINE + MIME_NEWLINE + m_attachmentData + MIME_NEWLINE + MIME_BOUNDARY_DASHES +
                   boundaryString;
}

std::string TestMimeAttachmentPart::getMimeString() const {
    return m_mimeString;
}

bool TestMimeAttachmentPart::validateMimeParsing() {
    auto attachmentId = m_attachmentManager->generateAttachmentId(m_contextId, m_contentId);
    auto reader = m_attachmentManager->createReader(attachmentId, avsCommon::utils::sds::ReaderPolicy::BLOCKING);

    std::vector<uint8_t> result(m_attachmentData.size());
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;
    auto numRead = reader->read(result.data(), result.size(), &readStatus);
    if (numRead != m_attachmentData.length()) {
        return false;
    }
    if (readStatus != InProcessAttachmentReader::ReadStatus::OK) {
        return false;
    }

    for (size_t i = 0; i < m_attachmentData.size(); ++i) {
        if (result[i] != m_attachmentData[i]) {
            return false;
        }
    }

    return true;
}

std::string constructTestMimeString(
    const std::vector<std::shared_ptr<TestMimePart>>& mimeParts,
    const std::string& boundaryString,
    bool addPrependedNewline) {
    std::string mimeString = (addPrependedNewline ? MIME_NEWLINE : "") + MIME_BOUNDARY_DASHES + boundaryString;

    for (auto mimePart : mimeParts) {
        mimeString += MIME_NEWLINE + mimePart->getMimeString();
    }

    // The final mime part needs the closing double dashes.
    mimeString += MIME_BOUNDARY_DASHES;

    return mimeString;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
