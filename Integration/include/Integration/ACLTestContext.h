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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_ACLTESTCONTEXT_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_ACLTESTCONTEXT_H_

#include <memory>
#include <string>

#include <ACL/Transport/MessageRouter.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <ContextManager/ContextManager.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "Integration/AuthDelegateTestContext.h"
#include "Integration/SDKTestContext.h"
#include "Integration/ConnectionStatusObserver.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * Class providing lifecycle management of resources needed for testing ACL or functionality that
 * requires ACL for testing.
 */
class ACLTestContext {
public:
    /**
     * Create an ACLTestContext.
     *
     * @note Only one instance of this class should exist at a time - but it is okay (and expected)
     * that multiple instances of this class will be created (and destroyed) during one execution
     * of the application using this class.
     *
     * Creating an instance of this class provides:
     * <li>Initialization of the @c Alexa @c Client @c SDK (includes @ libcurl and @c ConfigurationNode.</li>
     * <li>A @c CustomerDataManager instance.</li>
     * <li>An @c AuthDelegateInterface instance.</li>
     * <li>An @c AttachmentManager instance.</li>
     * <li>A @c ConnectionStatusObserver instance.</li>
     * <li>A @c ContextManager instance.</li>
     * <li>Initialization of @c PostConnect.</li>
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     * @return An ACLTestContext instance or nullptr if the operation failed.
     */
    static std::unique_ptr<ACLTestContext> create(const std::string& filePath, const std::string& overlay = "");

    /**
     * Destructor.  de-initializes all resources acquired during construction.
     */
    ~ACLTestContext();

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

    /**
     * Get the instance of @c AttachmentManager to use for the test.
     *
     * @return The instance of @c AttachmentManager to use for the test.
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> getAttachmentManager() const;

    /**
     * Get the @c MessageRouter instance to use for the test.
     *
     * @return The @c MessageRouter instance to use for the test.
     */
    std::shared_ptr<acl::MessageRouter> getMessageRouter() const;

    /**
     * Get the @c ConnectionStatusObserver instance to use for the test.
     *
     * @return The @c ConnectionStatusObserver instance to use for the test.
     */
    std::shared_ptr<ConnectionStatusObserver> getConnectionStatusObserver() const;

    /**
     * Get the @c ContextManager instance to use for the test.
     *
     * @return The @c ContextManager instance to use for the test.
     */
    std::shared_ptr<contextManager::ContextManager> getContextManager() const;

    /**
     * Wait for the @c ConnectionStatusObserver to be notified that the client has successfully connected to @c AVS.
     */
    void waitForConnected();

    /**
     * Wait for the @c ConnectionStatusObserver to be notified that the client has successfully disconnected
     * from @c AVS.
     */
    void waitForDisconnected();

private:
    /**
     * Constructor.
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     */
    ACLTestContext(const std::string& filePath, const std::string& overlay = "");

    /// Provide and AuthDelegate implementation suitable for testing.
    std::unique_ptr<AuthDelegateTestContext> m_authDelegateTestContext;

    /// The Object to use to manage attachments.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;

    /// Object that routs messages from @c AVS.
    std::shared_ptr<acl::MessageRouter> m_messageRouter;

    /// Object to monitor the status of the connection with @c AVS.
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;

    /// Object to acquire SDK context.
    std::shared_ptr<contextManager::ContextManager> m_contextManager;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_ACLTESTCONTEXT_H_
