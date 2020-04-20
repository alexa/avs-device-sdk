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

#include <vector>

#include <gtest/gtest.h>

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>

#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include "Integration/ACLTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace acl;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace authorization::cblAuthDelegate;
using namespace contextManager;
using namespace registrationManager;

std::unique_ptr<ACLTestContext> ACLTestContext::create(const std::string& filePath, const std::string& overlay) {
    std::unique_ptr<ACLTestContext> context(new ACLTestContext(filePath, overlay));
    if (context->m_authDelegateTestContext && context->m_attachmentManager && context->m_messageRouter &&
        context->m_connectionStatusObserver && context->m_contextManager) {
        return context;
    }
    return nullptr;
}

ACLTestContext::~ACLTestContext() {
    m_attachmentManager.reset();

    if (m_messageRouter) {
        m_messageRouter->shutdown();
    }

    m_connectionStatusObserver.reset();
    m_contextManager.reset();
    m_authDelegateTestContext.reset();
}

std::shared_ptr<AuthDelegateInterface> ACLTestContext::getAuthDelegate() const {
    return m_authDelegateTestContext->getAuthDelegate();
}

std::shared_ptr<CustomerDataManager> ACLTestContext::getCustomerDataManager() const {
    return m_authDelegateTestContext->getCustomerDataManager();
}

std::shared_ptr<AttachmentManager> ACLTestContext::getAttachmentManager() const {
    return m_attachmentManager;
}

std::shared_ptr<MessageRouter> ACLTestContext::getMessageRouter() const {
    return m_messageRouter;
}

std::shared_ptr<ConnectionStatusObserver> ACLTestContext::getConnectionStatusObserver() const {
    return m_connectionStatusObserver;
}

std::shared_ptr<ContextManager> ACLTestContext::getContextManager() const {
    return m_contextManager;
}

void ACLTestContext::waitForConnected() {
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::CONNECTED))
        << "Connecting timed out";
}

void ACLTestContext::waitForDisconnected() {
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::DISCONNECTED))
        << "Disconnecting timed out";
}

/// Default @c AVS gateway to connect to.
static const std::string DEFAULT_AVS_GATEWAY = "https://alexa.na.gateway.devices.a2z.com";

ACLTestContext::ACLTestContext(const std::string& filePath, const std::string& overlay) {
    m_authDelegateTestContext = AuthDelegateTestContext::create(filePath, overlay);
    EXPECT_TRUE(m_authDelegateTestContext);
    if (!m_authDelegateTestContext) {
        return;
    }

    auto config = ConfigurationNode::getRoot();
    EXPECT_TRUE(config);
    if (!config) {
        return;
    }

    auto deviceInfo = avsCommon::utils::DeviceInfo::create(config);
    EXPECT_TRUE(deviceInfo);
    if (!deviceInfo) {
        return;
    }

    m_contextManager = ContextManager::create(*deviceInfo);
    EXPECT_TRUE(m_contextManager);
    if (!m_contextManager) {
        return;
    }

    auto synchronizeStateSenderFactory =
        synchronizeStateSender::SynchronizeStateSenderFactory::create(m_contextManager);
    std::vector<std::shared_ptr<PostConnectOperationProviderInterface>> providers;
    providers.push_back(synchronizeStateSenderFactory);
    auto postConnectFactory = acl::PostConnectSequencerFactory::create(providers);
    auto http2ConnectionFactory = std::make_shared<LibcurlHTTP2ConnectionFactory>();
    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(http2ConnectionFactory, postConnectFactory);
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_messageRouter =
        std::make_shared<MessageRouter>(getAuthDelegate(), m_attachmentManager, transportFactory, DEFAULT_AVS_GATEWAY);
    m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
