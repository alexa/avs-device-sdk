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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MIMEUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MIMEUTILS_H_

#include <memory>
#include <string>
#include <vector>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>

#include "TestableMessageObserver.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Utility class to abstract the notion of testing a MIME part.
 */
class TestMimePart {
public:
    /**
     * Convert the data of this logical MIME part to an actual string which may be
     * used to feed a real MIME parser.
     *
     * @param boundaryString The boundary string for the MIME text.
     * @return The generated MIME string.
     */
    virtual std::string getMimeString() const = 0;

    /**
     * Function to validate the MIME part was parsed elsewhere and received
     * correctly.  Subclass specializations will expect to handle this in
     * different ways, using different internal objects.
     *
     * @return Whether the MIME part was parsed and received correctly.
     */
    virtual bool validateMimeParsing() = 0;
};

/**
 * A utility class to test a JSON MIME part, which our SDK interprets as
 * Directives.
 */
class TestMimeJsonPart : public TestMimePart {
public:
    /**
     * Constructor.
     *
     * @param boundaryString The boundary string for the MIME text.
     * @param dataSize The size of the directive string to be generated and
     * tested.
     * @param messageObserver The object which will expect to receive the
     * directive string once parsed elsewhere.
     */
    TestMimeJsonPart(
        const std::string& boundaryString,
        int dataSize,
        std::shared_ptr<TestableMessageObserver> messageObserver);

    /**
     * Constructor.
     *
     * @param mimeString The mime string for this part, including a leading
     * boundary
     * @param message The size of the directive string to be generated and tested.
     * @param messageObserver The object which will expect to receive the
     * directive string once parsed elsewhere.
     */
    TestMimeJsonPart(
        const std::string& mimeString,
        const std::string& message,
        std::shared_ptr<TestableMessageObserver> messageObserver);

    std::string getMimeString() const override;
    virtual bool validateMimeParsing() override;

private:
    /// The message text.
    std::string m_message;
    /// The observer which will expect to receive the message at some point during
    /// testing.
    std::shared_ptr<TestableMessageObserver> m_messageObserver;
    /// The mime string that should be parsed as m_message
    std::string m_mimeString;
};

/**
 * A utility class to test a binary MIME part, which our SDK interprets as
 * Attachments.
 */
class TestMimeAttachmentPart : public TestMimePart {
public:
    /**
     * Constructor.
     *
     * @param boundaryString The boundary string for the MIME text.
     * @param contextId The context id of the simulated Attachment.
     * @param contentId The content id of the simulated Attachment.
     * @param dataSize The size of the attachment data to be generated and tested.
     * @param attachmentManager An attachment manager with which this class should
     * interact.
     */
    TestMimeAttachmentPart(
        const std::string& boundaryString,
        const std::string& contextId,
        const std::string contentId,
        int dataSize,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    std::string getMimeString() const override;
    virtual bool validateMimeParsing() override;

private:
    /// The context id of the simulated Attachment.
    std::string m_contextId;
    /// The content id of the simulated Attachment.
    std::string m_contentId;
    /// The attachment data (we're using a string so it's human-readable if we
    /// need to debug this test code).
    std::string m_attachmentData;
    /// The AttachmentManager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    /// The mime string that should be parsed as m_attachmentData
    std::string m_mimeString;
};

/**
 * A utility function to generate a MIME string.
 *
 * @param mimeParts A vector of TestMimePart objects, which through polymorphism
 * know how to generate their own substrings.
 * @param boundaryString The boundary string to be used when generating the MIME
 * string.
 * @param addPrependedNewline true if a trailing newline (CRLF) sequence is required, false otherwise.
 * @return The generated MIME string.
 */
std::string constructTestMimeString(
    const std::vector<std::shared_ptr<TestMimePart>>& mimeParts,
    const std::string& boundaryString,
    bool addPrependedNewline = true);

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MIMEUTILS_H_
