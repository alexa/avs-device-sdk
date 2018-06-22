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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_AVSCOMMON_AVS_ATTACHMENT_MOCKATTACHMENTMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_AVSCOMMON_AVS_ATTACHMENT_MOCKATTACHMENTMANAGER_H_

#include <chrono>
#include <string>
#include <memory>

#include <gmock/gmock.h>
#include "AVSCommon/AVS/Attachment/AttachmentManagerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {
namespace test {

/// Mock class that implements the AttachmentManager.
class MockAttachmentManager : public AttachmentManagerInterface {
public:
    MOCK_CONST_METHOD2(generateAttachmentId, std::string(const std::string& contextId, const std::string& contentId));
    MOCK_METHOD1(setAttachmentTimeoutMinutes, bool(std::chrono::minutes timeoutMinutes));
    MOCK_METHOD2(
        createWriter,
        std::unique_ptr<AttachmentWriter>(const std::string& attachmentId, utils::sds::WriterPolicy policy));
    MOCK_METHOD2(
        createReader,
        std::unique_ptr<AttachmentReader>(const std::string& attachmentId, utils::sds::ReaderPolicy policy));
};

}  // namespace test
}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_AVSCOMMON_AVS_ATTACHMENT_MOCKATTACHMENTMANAGER_H_
