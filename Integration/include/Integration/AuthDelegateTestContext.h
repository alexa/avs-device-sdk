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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHDELEGATETESTCONTEXT_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHDELEGATETESTCONTEXT_H_

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <ACL/Transport/MessageRouter.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <ContextManager/ContextManager.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "Integration/ConnectionStatusObserver.h"
#include "Integration/SDKTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * Class providing lifecycle management of resources needed for testing instances of AuthDelegateInterface or
 * functionality that requires such instances for testing.
 */
class AuthDelegateTestContext {
public:
    /**
     * Create an AuthDelegateTestContext
     *
     * @note Only one instance of this class should exist at a time - but it is okay (and expected)
     * that multiple instances of this class will be created (and destroyed) during one execution
     * of the application using this class.
     *
     * Creating an instance of this class provides:
     * <li>A @c CustomerDataManager instance.</li>
     * <li>An @c AuthDelegateInterface instance.</li>
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     */
    static std::unique_ptr<AuthDelegateTestContext> create(
        const std::string& filePath,
        const std::string& overlay = "");

    /**
     * Destructor.  de-initializes all resources acquired during construction.
     */
    ~AuthDelegateTestContext();

    /**
     * Determine whether or not this instance was properly initialized.
     *
     * @return Whether or not this instance was properly initialized.
     */
    bool isValid() const;

    /**
     * Get the instance of @c AuthDelegateInterface to use for the test.
     *
     * @return The instance of @c AuthDelegateInterface to use for the test.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> getAuthDelegate() const;

    /**
     * Get the instance of @c CustomerDataManager to use for the test.
     *
     * @return The instance of @c CustomerDataManager to use for the test.
     */
    std::shared_ptr<registrationManager::CustomerDataManager> getCustomerDataManager() const;

private:
    /**
     * Implementation of @c CBLAuthRequesterInterface used to detect the case where the user
     * still needs to authorize access to @c AVS.
     */
    class AuthRequester : public authorization::cblAuthDelegate::CBLAuthRequesterInterface {
    public:
        /// @name CBLAuthRequesterInterface methods
        /// @{
        void onRequestAuthorization(const std::string& url, const std::string& code) override;
        void onCheckingForAuthorization() override;
        /// @}
    };

    /**
     * Constructor.
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     */
    AuthDelegateTestContext(const std::string& filePath, const std::string& overlay = "");

    /// Provide and SDK initialization suitable for testing.
    std::unique_ptr<SDKTestContext> m_sdkTestContext;

    /// The AuthDelegate to use for authorizing with LWA and AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// Object to manage customer specific data.
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHDELEGATETESTCONTEXT_H_
