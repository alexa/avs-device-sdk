/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file ServerDisconnectIntegrationTest.cpp

#include <chrono>
#include <fstream>
#include <thread>

#include <gtest/gtest.h>

#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSynchronizerFactory.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <ContextManager/ContextManager.h>

#include "Integration/AuthDelegateTestContext.h"
#include "Integration/AuthObserver.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/JsonHeader.h"
#include "Integration/ObservableMessageRequest.h"
#include "Integration/SDKTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace ::testing;
using namespace acl;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::libcurlUtils;

using alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface;

/// String to identify log entries originating from this file.
static const std::string TAG("ServerDisconnectIntegrationTest");
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The time to wait for expected message status on sending the message.
static const int TIMEOUT_FOR_SEND_IN_SECONDS = 10;

/// Path to the AlexaClientSDKConfig.json file (from command line arguments).
static std::string g_configPath;

/**
 * This class tests the functionality for communication between client and AVS using ACL library.
 */
class AVSCommunication : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create an AVSCommunication object which initializes @c m_connectionStatusObserver, @c m_avsConnectionManager,
     * @c m_authDelegate and @c m_messageRouter.
     *
     * @param authDelegate AuthDelegate to use to authorize access to @c AVS.
     * @return The created AVSCommunication object or @c nullptr if create fails.
     */
    static std::unique_ptr<AVSCommunication> create(std::shared_ptr<AuthDelegateInterface> authDelegate);

    /**
     * The function to establish connection by enabling the @c m_avsConnectionManager.
     */
    void connect();

    /**
     * The function to tear down the connection by disabling the @c m_avsConnectionManager.
     */
    void disconnect();

    /**
     * Function to retun the connection status observer.
     */
    std::shared_ptr<ConnectionStatusObserver> getConnectionStatusObserver();

    /**
     * The function to send one message to AVS.
     * @param jsonContent The content in json format to send in the message.
     * @param expectedStatus The expected status of the message being sent to AVS.
     * @param timeout The maximum time to wait for the @c expectedStatus.
     * @param attachmentReader The attachment reader for the MessageRequest.
     * @return true if expected avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status is received within the
     * @c timeout else false.
     */
    bool sendEvent(
        const std::string& jsonContent,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
        std::chrono::seconds timeout,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    /**
     * The function to check for Server Side Disconnect for the current connection.
     * @return true if disconnect is due to SERVER_SIDE_DISCONNECT else false.
     */
    bool checkForServerSideDisconnect();

protected:
    void doShutdown() override;

private:
    /**
     * AVSCommunication Constructor
     */
    AVSCommunication();
    /// ConnectionStatus Observer for checking the status changes sent to AVS.
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    /// Connection Manager for handling the communication between client.
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;
    /// ContextManager object.
    std::shared_ptr<contextManager::ContextManager> m_contextManager;
    /// Pointer to message router so we can properly shutdown
    std::shared_ptr<MessageRouter> m_messageRouter;
};

AVSCommunication::AVSCommunication() : RequiresShutdown("AVSCommunication") {
}

std::unique_ptr<AVSCommunication> AVSCommunication::create(std::shared_ptr<AuthDelegateInterface> authDelegate) {
    std::unique_ptr<AVSCommunication> avsCommunication(new AVSCommunication());
    if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAuthDelegate"));
        return nullptr;
    }
    avsCommunication->m_contextManager = contextManager::ContextManager::create();
    avsCommunication->m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();

    auto postConnectFactory = acl::PostConnectSynchronizerFactory::create(avsCommunication->m_contextManager);
    auto http2ConnectionFactory = std::make_shared<LibcurlHTTP2ConnectionFactory>();
    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(http2ConnectionFactory, postConnectFactory);
    avsCommunication->m_messageRouter = std::make_shared<MessageRouter>(
        authDelegate,
        std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS),
        transportFactory);

    avsCommunication->m_avsConnectionManager = AVSConnectionManager::create(
        avsCommunication->m_messageRouter,
        false,
        {avsCommunication->m_connectionStatusObserver},
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
    if (!avsCommunication->m_avsConnectionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAVSConnectionManager"));
        return nullptr;
    }
    return avsCommunication;
}

void AVSCommunication::connect() {
    m_avsConnectionManager->enable();

    /*
    Cannot wait anymore for status to move to connected state
    AVS could kick one the comm's out even before reaching CONNECTED
    state when post-connect sends the context with its profile.
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(
                ConnectionStatusObserverInterface::Status::CONNECTED));
    */
}

void AVSCommunication::disconnect() {
    m_avsConnectionManager->disable();
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::DISCONNECTED));
}

std::shared_ptr<ConnectionStatusObserver> AVSCommunication::getConnectionStatusObserver() {
    return m_connectionStatusObserver;
}

void AVSCommunication::doShutdown() {
    m_avsConnectionManager->shutdown();
    m_messageRouter->shutdown();
}

bool AVSCommunication::sendEvent(
    const std::string& jsonContent,
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status expectedStatus,
    std::chrono::seconds timeout,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) {
    auto messageRequest = std::make_shared<ObservableMessageRequest>(jsonContent, attachmentReader);
    m_avsConnectionManager->sendMessage(messageRequest);
    return messageRequest->waitFor(expectedStatus, timeout);
}

bool AVSCommunication::checkForServerSideDisconnect() {
    return m_connectionStatusObserver->checkForServerSideDisconnect();
}

/**
 * Class to implement the integration test of Server Side Disconnect between two connections with same configuration.
 */
class ServerDisconnectIntegrationTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    /// Context for running AuthDelegate based tests.
    std::unique_ptr<AuthDelegateTestContext> m_authDelegateTestContext;
    /// Object for the 1st connection to AVS.
    std::unique_ptr<AVSCommunication> m_firstAvsCommunication;
    /// Object for the 2nd connection to AVS.
    std::unique_ptr<AVSCommunication> m_secondAvsCommunication;
};

void ServerDisconnectIntegrationTest::SetUp() {
    m_authDelegateTestContext = AuthDelegateTestContext::create(g_configPath);
    ASSERT_TRUE(m_authDelegateTestContext);

    auto authDelegate = m_authDelegateTestContext->getAuthDelegate();
    ASSERT_TRUE(m_firstAvsCommunication = AVSCommunication::create(authDelegate));
    ASSERT_TRUE(m_secondAvsCommunication = AVSCommunication::create(authDelegate));
}

void ServerDisconnectIntegrationTest::TearDown() {
    // Note that these nullptr checks are needed to avoid segaults if @c SetUp() failed.
    if (m_firstAvsCommunication) {
        m_firstAvsCommunication->shutdown();
    }
    if (m_secondAvsCommunication) {
        m_secondAvsCommunication->shutdown();
    }
    m_authDelegateTestContext.reset();
}

/**
 * Test if server side disconnect occurs by trying to connect to AVS using the same configuration
 * using two different @c AVSConnectionManager.
 */
TEST_F(ServerDisconnectIntegrationTest, testConnect) {
    m_firstAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));

    m_secondAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::PENDING));
    EXPECT_TRUE(m_firstAvsCommunication->checkForServerSideDisconnect());

    m_firstAvsCommunication->disconnect();
    m_secondAvsCommunication->disconnect();
}

/**
 * Test if server side disconnect occurs by trying to connect to AVS using the same configuration
 * using two different @c AVSConnectionManager and reconnecting one of the connections.
 */
TEST_F(ServerDisconnectIntegrationTest, testReConnect) {
    m_firstAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));

    m_secondAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::PENDING));
    EXPECT_TRUE(m_firstAvsCommunication->checkForServerSideDisconnect());

    m_firstAvsCommunication->disconnect();
    m_secondAvsCommunication->disconnect();

    m_firstAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));
    m_firstAvsCommunication->disconnect();

    m_secondAvsCommunication->connect();
    ASSERT_TRUE(m_secondAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));
    m_secondAvsCommunication->disconnect();
}

/**
 * Test sending a message while having a server side disconnect. The send fails to return the expected
 * status of SUCCESS.
 */
TEST_F(ServerDisconnectIntegrationTest, testSendEvent) {
    m_firstAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));

    m_secondAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::PENDING));
    EXPECT_TRUE(m_firstAvsCommunication->checkForServerSideDisconnect());

    m_firstAvsCommunication->disconnect();
    m_secondAvsCommunication->disconnect();

    m_firstAvsCommunication->connect();
    ASSERT_TRUE(m_firstAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));

    ASSERT_TRUE(m_firstAvsCommunication->sendEvent(
        SYNCHRONIZE_STATE_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        std::chrono::seconds(TIMEOUT_FOR_SEND_IN_SECONDS)));
    m_firstAvsCommunication->disconnect();

    m_secondAvsCommunication->connect();
    ASSERT_TRUE(m_secondAvsCommunication->getConnectionStatusObserver()->waitFor(
        ConnectionStatusObserverInterface::Status::CONNECTED));

    ASSERT_TRUE(m_secondAvsCommunication->sendEvent(
        SYNCHRONIZE_STATE_JSON,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT,
        std::chrono::seconds(TIMEOUT_FOR_SEND_IN_SECONDS)));
    m_secondAvsCommunication->disconnect();
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path_to_AlexaClientSDKConfig.json>" << std::endl;
        return 1;
    } else {
        alexaClientSDK::integration::test::g_configPath = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
