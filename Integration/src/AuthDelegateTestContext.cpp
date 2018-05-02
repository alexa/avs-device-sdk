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

#include <gtest/gtest.h>

#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>

#include "Integration/AuthDelegateTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace authorization::cblAuthDelegate;
using namespace contextManager;
using namespace registrationManager;
using namespace ::testing;

std::unique_ptr<AuthDelegateTestContext> AuthDelegateTestContext::create(
    const std::string& filePath,
    const std::string& overlay) {
    std::unique_ptr<AuthDelegateTestContext> context(new AuthDelegateTestContext(filePath, overlay));
    if (context->m_sdkTestContext && context->m_customerDataManager && context->m_authDelegate) {
        return context;
    }
    return nullptr;
}

AuthDelegateTestContext::~AuthDelegateTestContext() {
    m_authDelegate.reset();
    m_customerDataManager.reset();
    m_sdkTestContext.reset();
}

void AuthDelegateTestContext::AuthRequester::onRequestAuthorization(const std::string& url, const std::string& code) {
    ASSERT_FALSE(true) << "FATAL ERROR: Authorization required before running integration test";
}

void AuthDelegateTestContext::AuthRequester::onCheckingForAuthorization() {
}

bool AuthDelegateTestContext::isValid() const {
    return m_customerDataManager && m_authDelegate;
}

std::shared_ptr<AuthDelegateInterface> AuthDelegateTestContext::getAuthDelegate() const {
    return m_authDelegate;
}

std::shared_ptr<registrationManager::CustomerDataManager> AuthDelegateTestContext::getCustomerDataManager() const {
    return m_customerDataManager;
}

AuthDelegateTestContext::AuthDelegateTestContext(const std::string& filePath, const std::string& overlay) {
    m_sdkTestContext = SDKTestContext::create(filePath, overlay);
    EXPECT_TRUE(m_sdkTestContext);
    if (!m_sdkTestContext) {
        return;
    }

    auto config = ConfigurationNode::getRoot();
    EXPECT_TRUE(config);
    if (!config) {
        return;
    }

    // TODO: use configuration or parameter here to determine which kind of AuthDelegate to create.

    m_customerDataManager = std::make_shared<CustomerDataManager>();
    EXPECT_TRUE(m_customerDataManager);
    if (!m_customerDataManager) {
        return;
    }

    auto storage = SQLiteCBLAuthDelegateStorage::create(config);
    EXPECT_TRUE(storage);
    if (!storage) {
        return;
    }

    m_authDelegate =
        CBLAuthDelegate::create(config, m_customerDataManager, std::move(storage), std::make_shared<AuthRequester>());
    EXPECT_TRUE(m_authDelegate);
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
