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
#ifndef ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKMESSAGESTORAGE_H_
#define ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKMESSAGESTORAGE_H_

#include <gmock/gmock.h>

#include <CertifiedSender/MessageStorageInterface.h>

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

/**
 *  Mock class that implements the MessageStorageInterface.
 */
class MockMessageStorage : public MessageStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(store, bool(const std::string& message, int* id));
    MOCK_METHOD1(load, bool(std::queue<StoredMessage>* messageContainer));
    MOCK_METHOD1(erase, bool(int messageId));
    MOCK_METHOD0(clearDatabase, bool());

private:
};

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKMESSAGESTORAGE_H_
