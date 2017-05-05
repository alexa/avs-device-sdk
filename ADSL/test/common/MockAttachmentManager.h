/*
 * MockAttachmentManager.h
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

#ifndef ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_ATTACHMENT_MANAGER_H_
#define ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_ATTACHMENT_MANAGER_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/AttachmentManagerInterface.h>

namespace alexaClientSDK {
namespace adsl {
namespace test {

/**
 * MockAttachmentManager used when creating @c AVSDirectives.
 */
class MockAttachmentManager : public avsCommon::AttachmentManagerInterface {
public:
    MOCK_METHOD1(createAttachmentReader, std::future<std::shared_ptr<std::iostream>> (const std::string& attachmentId));
    MOCK_METHOD2(createAttachment, void (const std::string& attachmentId, std::shared_ptr<std::iostream> attachment));
    MOCK_METHOD1(releaseAttachment, void (const std::string& attachmentId));
};

} // namespace test
} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_ATTACHMENT_MANAGER_H_
