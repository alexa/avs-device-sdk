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

#include <gtest/gtest.h>

#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#ifdef ACSDK_ACS_UTILS
#include <acsdkFFSAuthDelegate/ACSFFSAuthDelegateStorage.h>
#include <acsdkFFSAuthDelegate/FFSAuthDelegate.h>
#else
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#endif
#include <RegistrationManager/CustomerDataManagerFactory.h>

#include "Integration/AuthDelegateTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
#ifdef ACSDK_ACS_UTILS
using namespace acsdkffsAuthDelegate;
#else
using namespace authorization::cblAuthDelegate;
#endif
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

#ifdef ACSDK_ACS_UTILS
void AuthDelegateTestContext::AuthRequester::onRequestFFS() {
}
#else
void AuthDelegateTestContext::AuthRequester::onRequestAuthorization(const std::string& url, const std::string& code) {
    ASSERT_FALSE(true) << "FATAL ERROR: Authorization required before running integration test";
}

void AuthDelegateTestContext::AuthRequester::onCheckingForAuthorization() {
}
#endif

bool AuthDelegateTestContext::isValid() const {
    return m_customerDataManager && m_authDelegate;
}

std::shared_ptr<AuthDelegateInterface> AuthDelegateTestContext::getAuthDelegate() const {
    return m_authDelegate;
}

std::shared_ptr<registrationManager::CustomerDataManagerInterface> AuthDelegateTestContext::getCustomerDataManager()
    const {
    return m_customerDataManager;
}

AuthDelegateTestContext::AuthDelegateTestContext(const std::string& filePath, const std::string& overlay) {
    m_sdkTestContext = SDKTestContext::create(filePath, overlay);
    EXPECT_TRUE(m_sdkTestContext);
    if (!m_sdkTestContext) {
        return;
    }

    auto config = std::make_shared<ConfigurationNode>(ConfigurationNode::getRoot());
    EXPECT_TRUE(*config);
    if (!*config) {
        return;
    }

    // TODO: use configuration or parameter here to determine which kind of AuthDelegate to create.

    m_customerDataManager = registrationManager::CustomerDataManagerFactory::createCustomerDataManagerInterface();
    EXPECT_TRUE(m_customerDataManager);
    if (!m_customerDataManager) {
        return;
    }

#ifdef ACSDK_ACS_UTILS
    auto storage = ACSFFSAuthDelegateStorage::createFFSAuthDelegateStorageInterface();
    EXPECT_TRUE(storage);
    if (!storage) {
        return;
    }

    m_authDelegate = FFSAuthDelegate::createFFSAuthDelegateInterface(
        config,
        m_customerDataManager,
        std::move(storage),
        std::make_shared<AuthRequester>(),
        avsCommon::utils::libcurlUtils::HttpPost::create(),
        avsCommon::utils::DeviceInfo::createFromConfiguration(config));

    EXPECT_TRUE(m_authDelegate);
#else
    auto storage = SQLiteCBLAuthDelegateStorage::createCBLAuthDelegateStorageInterface(config, nullptr, nullptr);
    EXPECT_TRUE(storage);
    if (!storage) {
        return;
    }

    m_authDelegate = CBLAuthDelegate::createAuthDelegateInterface(
        config,
        m_customerDataManager,
        std::move(storage),
        std::make_shared<AuthRequester>(),
        avsCommon::utils::libcurlUtils::HttpPost::create(),
        avsCommon::utils::DeviceInfo::createFromConfiguration(config));

    EXPECT_TRUE(m_authDelegate);
#endif
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
