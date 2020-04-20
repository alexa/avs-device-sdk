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

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockPostConnectOperation.h"
#include "MockPostConnectOperationProvider.h"
#include <ACL/Transport/PostConnectSequencerFactory.h>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace ::testing;

/**
 * Test harness for @c PostConnectSequencerFactory.
 */
class PostConnectSequencerFactoryTest : public Test {};

/**
 * Test if create returns nullptr when one of the providers is a nullptr.
 */
TEST_F(PostConnectSequencerFactoryTest, test_createWithNullProviders) {
    std::vector<std::shared_ptr<PostConnectOperationProviderInterface>> providers;

    auto provider1 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    auto provider2 = nullptr;
    auto provider3 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    providers.push_back(provider1);
    providers.push_back(provider2);
    providers.push_back(provider3);

    auto instance = PostConnectSequencerFactory::create(providers);
    ASSERT_EQ(instance, nullptr);
}

/**
 * Test if the createPostConnectOperation() is called on all the providers when createPostConnect() is called.
 */
TEST_F(PostConnectSequencerFactoryTest, test_createPostConnectCallsProviders) {
    std::vector<std::shared_ptr<PostConnectOperationProviderInterface>> providers;

    auto provider1 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    auto provider2 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    auto provider3 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    providers.push_back(provider1);
    providers.push_back(provider2);
    providers.push_back(provider3);

    EXPECT_CALL(*provider1, createPostConnectOperation()).Times(1);
    EXPECT_CALL(*provider2, createPostConnectOperation()).Times(1);
    EXPECT_CALL(*provider3, createPostConnectOperation()).Times(1);

    auto instance = PostConnectSequencerFactory::create(providers);
    ASSERT_NE(instance->createPostConnect(), nullptr);
}

/**
 * Test createPostConnect() does not fail if one of the providers returns nullptr.
 */
TEST_F(PostConnectSequencerFactoryTest, test_createPostConnectWhenProviderReturnsNull) {
    std::vector<std::shared_ptr<PostConnectOperationProviderInterface>> providers;

    auto provider1 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    auto provider2 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    auto provider3 = std::make_shared<NiceMock<MockPostConnectOperationProvider>>();
    providers.push_back(provider1);
    providers.push_back(provider2);
    providers.push_back(provider3);
    auto operation1 = std::make_shared<NiceMock<MockPostConnectOperation>>();
    auto operation3 = std::make_shared<NiceMock<MockPostConnectOperation>>();

    EXPECT_CALL(*provider1, createPostConnectOperation()).Times(1).WillOnce(Return(operation1));
    EXPECT_CALL(*provider2, createPostConnectOperation()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*provider3, createPostConnectOperation()).Times(1).WillOnce(Return(operation3));

    auto instance = PostConnectSequencerFactory::create(providers);
    ASSERT_NE(instance->createPostConnect(), nullptr);
}

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK