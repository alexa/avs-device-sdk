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

/// @file PostConnectOperationProviderRegistrarTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkPostConnectOperationProviderRegistrar/PostConnectOperationProviderRegistrar.h"

namespace alexaClientSDK {
namespace acsdkPostConnectOperationProviderRegistrar {
namespace test {

using namespace ::testing;
using namespace acsdkStartupManagerInterfaces;
using namespace avsCommon::sdkInterfaces;

class MockStartupNotifier : public StartupNotifierInterface {
public:
    MOCK_METHOD1(addObserver, void(const std::shared_ptr<RequiresStartupInterface>& observer));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<RequiresStartupInterface>& observer));
    MOCK_METHOD1(addWeakPtrObserver, void(const std::weak_ptr<RequiresStartupInterface>& observer));
    MOCK_METHOD1(removeWeakPtrObserver, void(const std::weak_ptr<RequiresStartupInterface>& observer));
    MOCK_METHOD1(notifyObservers, void(std::function<void(const std::shared_ptr<RequiresStartupInterface>&)>));
    MOCK_METHOD1(notifyObserversInReverse, bool(std::function<void(const std::shared_ptr<RequiresStartupInterface>&)>));
    MOCK_METHOD1(setAddObserverFunction, void(std::function<void(const std::shared_ptr<RequiresStartupInterface>&)>));
};

class MockPostConnectOperationProvider : public PostConnectOperationProviderInterface {
public:
    MOCK_METHOD0(
        createPostConnectOperation,
        std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>());
};

/**
 * Verify the simplest failure case - null StartupNotifier.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_simplestFailureCase) {
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(nullptr);
    ASSERT_FALSE(registrar);
}

/**
 * Verify the simplest success case - non-null StartupNotifier.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_simplestSuccessCase) {
    auto startupNotifier = std::make_shared<MockStartupNotifier>();
    EXPECT_CALL(*startupNotifier, addObserver(_)).Times(1);
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(startupNotifier);
    ASSERT_TRUE(registrar);
    // Request before startup should not have a value.
    ASSERT_FALSE(registrar->getProviders().hasValue());
}

/**
 * Verify getProviders() returns no value before startup even if providers were added.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_getProvidersBeforeStartup) {
    auto startupNotifier = std::make_shared<MockStartupNotifier>();
    EXPECT_CALL(*startupNotifier, addObserver(_)).Times(1);
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(startupNotifier);
    ASSERT_TRUE(registrar);
    auto provider1 = std::make_shared<MockPostConnectOperationProvider>();
    auto provider2 = std::make_shared<MockPostConnectOperationProvider>();
    ASSERT_TRUE(registrar->registerProvider(provider1));
    ASSERT_TRUE(registrar->registerProvider(provider2));
    // Request before startup should not have a value.
    ASSERT_FALSE(registrar->getProviders().hasValue());
}

/**
 * Verify getProviders() after startup but no registrations returns an empty collection of providers.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_getEmptyProvidersAfterStartup) {
    std::shared_ptr<RequiresStartupInterface> requiresStartup;
    auto startupNotifier = std::make_shared<MockStartupNotifier>();
    EXPECT_CALL(*startupNotifier, addObserver(_))
        .WillOnce(
            Invoke([&requiresStartup](const std::shared_ptr<RequiresStartupInterface>& in) { requiresStartup = in; }));
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(startupNotifier);
    ASSERT_TRUE(registrar);
    ASSERT_TRUE(requiresStartup);
    ASSERT_TRUE(requiresStartup->startup());
    // Request after startup should have a value: an empty collection of providers.
    auto providers = registrar->getProviders();
    ASSERT_TRUE(providers.hasValue());
    ASSERT_TRUE(providers.value().empty());
}

/**
 * Verify getProviders() after registration of providers and startup returns a collection of providers.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_getProvidersAfterStartup) {
    std::shared_ptr<RequiresStartupInterface> requiresStartup;
    auto startupNotifier = std::make_shared<MockStartupNotifier>();
    EXPECT_CALL(*startupNotifier, addObserver(_))
        .WillOnce(
            Invoke([&requiresStartup](const std::shared_ptr<RequiresStartupInterface>& in) { requiresStartup = in; }));
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(startupNotifier);
    ASSERT_TRUE(registrar);
    ASSERT_TRUE(requiresStartup);
    auto provider1 = std::make_shared<MockPostConnectOperationProvider>();
    auto provider2 = std::make_shared<MockPostConnectOperationProvider>();
    ASSERT_TRUE(registrar->registerProvider(provider1));
    ASSERT_TRUE(registrar->registerProvider(provider2));
    ASSERT_TRUE(requiresStartup->startup());
    // Request after startup should have a value: a collection two providers.
    auto providers = registrar->getProviders();
    ASSERT_TRUE(providers.hasValue());
    ASSERT_EQ(providers.value().size(), 2U);
}

/**
 * Verify registering providers after startup fails.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_registerProvidersAfterStartup) {
    std::shared_ptr<RequiresStartupInterface> requiresStartup;
    auto startupNotifier = std::make_shared<MockStartupNotifier>();
    EXPECT_CALL(*startupNotifier, addObserver(_))
        .WillOnce(
            Invoke([&requiresStartup](const std::shared_ptr<RequiresStartupInterface>& in) { requiresStartup = in; }));
    auto registrar =
        PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface(startupNotifier);
    ASSERT_TRUE(registrar);
    ASSERT_TRUE(requiresStartup);
    auto provider1 = std::make_shared<MockPostConnectOperationProvider>();
    ASSERT_TRUE(registrar->registerProvider(provider1));
    ASSERT_TRUE(requiresStartup->startup());
    // Verify register after startup fails.
    auto provider2 = std::make_shared<MockPostConnectOperationProvider>();
    ASSERT_FALSE(registrar->registerProvider(provider2));
    // Request after startup should have a value: a collection with one provider.
    auto providers = registrar->getProviders();
    ASSERT_TRUE(providers.hasValue());
    ASSERT_EQ(providers.value().size(), 1U);
}

}  // namespace test
}  // namespace acsdkPostConnectOperationProviderRegistrar
}  // namespace alexaClientSDK
