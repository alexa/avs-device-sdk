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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTMANAGER_H_

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * A version of the Decorator Pattern, this class allows us to return a special AttachmentWriter class to
 * calling code, while routing all other functionality to a normal encapsulated AttachmentManager object.
 */
class TestableAttachmentManager : public avsCommon::avs::attachment::AttachmentManager {
public:
    /**
     * Constructor.
     */
    TestableAttachmentManager();

    std::string generateAttachmentId(const std::string& contextId, const std::string& contentId) const override;

    bool setAttachmentTimeoutMinutes(std::chrono::minutes timeoutMinutes) override;

    std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> createWriter(
        const std::string& attachmentId,
        avsCommon::utils::sds::WriterPolicy policy) override;

    std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> createReader(
        const std::string& attachmentId,
        avsCommon::utils::sds::ReaderPolicy policy) override;

private:
    /// The real AttachmentManager that most functionality routes to.
    std::unique_ptr<avsCommon::avs::attachment::AttachmentManager> m_manager;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTMANAGER_H_
