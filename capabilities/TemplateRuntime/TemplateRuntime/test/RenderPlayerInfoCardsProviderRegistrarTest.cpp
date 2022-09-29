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

/// @file RenderPlayerInfoCardsProviderRegistrarTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>

#include <acsdk/TemplateRuntime/RenderPlayerInfoCardsProviderRegistrarFactory.h>

namespace alexaClientSDK {
namespace templateRuntime {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;

class MockRenderPlayerInfoCardProvider : public RenderPlayerInfoCardsProviderInterface {
public:
    MOCK_METHOD1(setObserver, void(std::shared_ptr<RenderPlayerInfoCardsObserverInterface>));
};

/**
 * Verify getProviders() returns the set of registered providers.
 */
TEST(RenderPlayerInfoCardsProviderTest, test_getProviders) {
    auto provider1 = std::make_shared<StrictMock<MockRenderPlayerInfoCardProvider>>();
    auto provider2 = std::make_shared<StrictMock<MockRenderPlayerInfoCardProvider>>();
    auto registrar = createRenderPlayerInfoCardsProviderRegistrarInterface();

    ASSERT_TRUE(registrar);

    ASSERT_TRUE(registrar->registerProvider(provider1));
    ASSERT_TRUE(registrar->registerProvider(provider2));

    std::unordered_set<std::shared_ptr<RenderPlayerInfoCardsProviderInterface>> providers = registrar->getProviders();
    ASSERT_THAT(providers, Contains(provider1));
    ASSERT_THAT(providers, Contains(provider2));
}

/**
 * Verify registering a provider fails if it is a duplicate.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_registerDuplicateProviderFails) {
    auto provider1 = std::make_shared<StrictMock<MockRenderPlayerInfoCardProvider>>();
    auto registrar = createRenderPlayerInfoCardsProviderRegistrarInterface();

    ASSERT_TRUE(registrar);

    ASSERT_TRUE(registrar->registerProvider(provider1));
    ASSERT_FALSE(registrar->registerProvider(provider1));

    std::unordered_set<std::shared_ptr<RenderPlayerInfoCardsProviderInterface>> providers = registrar->getProviders();
    ASSERT_TRUE(providers.size() == 1);
    ASSERT_THAT(providers, Contains(provider1));
}

/**
 * Verify registering a null provider fails.
 */
TEST(PostConnectOperationProviderRegistrarTest, test_registerNullProviderFails) {
    auto registrar = createRenderPlayerInfoCardsProviderRegistrarInterface();

    ASSERT_TRUE(registrar);

    ASSERT_FALSE(registrar->registerProvider(nullptr));

    std::unordered_set<std::shared_ptr<RenderPlayerInfoCardsProviderInterface>> providers = registrar->getProviders();
    ASSERT_TRUE(providers.size() == 0);
}

}  // namespace test
}  // namespace templateRuntime
}  // namespace alexaClientSDK
