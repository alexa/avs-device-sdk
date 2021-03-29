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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/EditableMessageRequest.h"

using namespace ::testing;

namespace alexaClientSDK {

namespace avsCommon {
namespace avs {
namespace test {

class MockAttachmentReader : public attachment::AttachmentReader {
public:
    MOCK_METHOD4(
        read,
        std::size_t(
            void* buf,
            std::size_t numBytes,
            attachment::AttachmentReader::ReadStatus* readStatus,
            std::chrono::milliseconds timeoutMs));
    MOCK_METHOD1(seek, bool(uint64_t offset));
    MOCK_METHOD0(getNumUnreadBytes, uint64_t());
    MOCK_METHOD1(close, void(attachment::AttachmentReader::ClosePoint closePoint));
};

class EditableMessageRequestTest : public ::testing::Test {};

TEST_F(EditableMessageRequestTest, test_setJsonContent) {
    std::string jsonContent = "{\"name\": \"value\"}";
    MessageRequest sourceRequest("{}", true, "", {});
    EditableMessageRequest messageRequest = sourceRequest;

    EXPECT_NE(jsonContent, messageRequest.getJsonContent());
    messageRequest.setJsonContent(jsonContent);
    EXPECT_EQ(jsonContent, messageRequest.getJsonContent());
}

TEST_F(EditableMessageRequestTest, test_setAttachmentReaders) {
    std::shared_ptr<attachment::AttachmentReader> reader = std::make_shared<MockAttachmentReader>();
    std::vector<std::shared_ptr<MessageRequest::NamedReader>> attachmentReaders{
        std::make_shared<MessageRequest::NamedReader>("Test", reader)};
    MessageRequest sourceRequest("{}", true, "", {});
    EditableMessageRequest messageRequest = sourceRequest;

    EXPECT_EQ(0, messageRequest.attachmentReadersCount());
    messageRequest.setAttachmentReaders(attachmentReaders);
    EXPECT_EQ(1, messageRequest.attachmentReadersCount());
    auto namedReader = messageRequest.getAttachmentReader(0);
    EXPECT_TRUE(namedReader != nullptr);
    EXPECT_EQ(attachmentReaders[0]->name, namedReader->name);
    EXPECT_EQ(attachmentReaders[0]->reader, namedReader->reader);
}

TEST_F(EditableMessageRequestTest, test_setAttachmentReadersFails) {
    auto reader = std::make_shared<MockAttachmentReader>();
    auto validNamedReader = std::make_shared<MessageRequest::NamedReader>("Test", reader);
    std::vector<std::shared_ptr<MessageRequest::NamedReader>> attachmentReaders{
        validNamedReader, nullptr, std::make_shared<MessageRequest::NamedReader>("Test2", nullptr)};
    MessageRequest sourceRequest("{}", true, "", {});
    EditableMessageRequest messageRequest = sourceRequest;

    EXPECT_EQ(0, messageRequest.attachmentReadersCount());
    messageRequest.setAttachmentReaders(attachmentReaders);
    EXPECT_EQ(1, messageRequest.attachmentReadersCount());
    auto namedReader = messageRequest.getAttachmentReader(0);
    EXPECT_TRUE(namedReader != nullptr);
    EXPECT_EQ(attachmentReaders[0]->name, namedReader->name);
    EXPECT_EQ(attachmentReaders[0]->reader, namedReader->reader);
}

TEST_F(EditableMessageRequestTest, test_setMessageRequestResolveFunction) {
    MessageRequest sourceRequest("{}", true, "", {});
    EditableMessageRequest request = sourceRequest;
    MockFunction<bool(const std::shared_ptr<EditableMessageRequest>&, std::string)> mockResolverFunc;

    EXPECT_TRUE(request.isResolved());
    request.setMessageRequestResolveFunction(mockResolverFunc.AsStdFunction());
    EXPECT_FALSE(request.isResolved());
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
