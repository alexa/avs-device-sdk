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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGECONSUMER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGECONSUMER_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ACL/Transport/MessageConsumerInterface.h>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * A mock class of @c MessageConsumerInterface.
 */
class MockMessageConsumer : public MessageConsumerInterface {
public:
    MOCK_METHOD2(consumeMessage, void(const std::string& contextId, const std::string& message));
};

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGECONSUMER_H_ */
