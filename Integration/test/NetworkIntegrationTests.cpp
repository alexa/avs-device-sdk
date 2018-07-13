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

/// @file NetworkIntegrationTests.cpp

#include <fstream>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/HTTP2MessageRouter.h>
#include <ContextManager/ContextManager.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AuthDelegate/AuthDelegate.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Integration/AuthObserver.h"
#include "Integration/ConnectionStatusObserver.h"
#include "Integration/JsonHeader.h"
#include "Integration/ObservableMessageRequest.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace ::testing;
using namespace acl;
using namespace authDelegate;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces;

static const std::string TAG("NetworkIntegrationTests");

#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Amount of Delay in milliseconds to be added.
static const std::string DELAY_TIME = "1000ms";
/// Amount of Delay for causing a TIMEDOUT status in MessageRequest.
static const std::string LONG_DELAY_TIME = "40000ms";
/// The time to wait for expected message status on sending the message.
static const int TIMEOUT_FOR_SEND_IN_SECONDS = 10;
/// The time to wait for expected message status when delay is longer.
static const int LONG_TIMEOUT_FOR_SEND_IN_SECONDS = 40;

/// Path to the AlexaClientSDKConfig.json file.
std::string g_configPath;
/// The network interface specified for the delay to be added.
std::string network_interface;

/**
 * Test class to stress test the ACL Library for slow network connection.
 */
class NetworkIntegrationTests : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /**
     * This function enables the @c m_avsConnectionManager to establish a connection to AVS.
     */
    void connect();

    /**
     * This function disables the @c m_avsConnectionManager to tear down the connection.
     */
    void disconnect();

    /**
     * The function to send one message to AVS.
     * @param jsonContent The content in json format to send in the message.
     * @param expectedStatus The expected status of the message being sent to AVS.
     * @param timeout The maximum time to wait for the @c expectedStatus.
     * @param attachmentReader The attachment reader for the MessageRequest.
     */
    void sendEvent(
        const std::string& jsonContent,
        MessageRequest::Status expectedStatus,
        std::chrono::seconds timeout,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    /**
     * The function adds delay to the network.
     * @param DelayTime The delay in milliseconds to be introduced in the network.
     */
    void addDelay(const std::string& delayTime);

    /**
     * Remove the delay in the network.
     */
    void deleteDelay();

protected:
    /// ConnectionStatus Observer for checking the status changes in connection.
    std::shared_ptr<ConnectionStatusObserver> m_connectionStatusObserver;
    /// Connection Manager for handling the communication between client and AVS.
    std::shared_ptr<AVSConnectionManager> m_avsConnectionManager;

    /// ContextManager object.
    std::shared_ptr<contextManager::ContextManager> m_contextManager;

private:
    /// AuthObserver for checking the status of authorization.
    std::shared_ptr<AuthObserver> m_authObserver;
};

void NetworkIntegrationTests::SetUp() {
    std::ifstream infile(g_configPath);
    ASSERT_TRUE(infile.good());
    ASSERT_TRUE(AlexaClientSDKInit::initialize({&infile}));

    /// AuthDelegate to handle the authorization token.
    std::shared_ptr<AuthDelegate> authDelegate;
    /// MessageRouter for routing the message to @c HTTPTransport.
    std::shared_ptr<MessageRouter> messageRouter;

    m_authObserver = std::make_shared<AuthObserver>();
    ASSERT_TRUE(authDelegate = AuthDelegate::create());
    authDelegate->addAuthObserver(m_authObserver);
    m_connectionStatusObserver = std::make_shared<ConnectionStatusObserver>();
    messageRouter = std::make_shared<HTTP2MessageRouter>(authDelegate, nullptr);

    m_contextManager = contextManager::ContextManager::create();
    ASSERT_NE(m_contextManager, nullptr);
    PostConnectObject::init(m_contextManager);

    bool isEnabled = false;
    m_avsConnectionManager = AVSConnectionManager::create(
        messageRouter,
        isEnabled,
        {m_connectionStatusObserver},
        std::unordered_set<std::shared_ptr<MessageObserverInterface>>());
    ASSERT_NE(m_avsConnectionManager, nullptr);
}

void NetworkIntegrationTests::TearDown() {
    AlexaClientSDKInit::uninitialize();
    deleteDelay();
}

void NetworkIntegrationTests::connect() {
    ASSERT_TRUE(m_authObserver->waitFor(AuthObserver::State::REFRESHED));
    m_avsConnectionManager->enable();
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::CONNECTED));
}

void NetworkIntegrationTests::disconnect() {
    m_avsConnectionManager->disable();
    ASSERT_TRUE(m_connectionStatusObserver->waitFor(ConnectionStatusObserverInterface::Status::DISCONNECTED))
        << "Disconnecting timed out.";
}

void NetworkIntegrationTests::sendEvent(
    const std::string& jsonContent,
    MessageRequest::Status expectedStatus,
    std::chrono::seconds timeout,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader) {
    auto messageRequest = std::make_shared<ObservableMessageRequest>(jsonContent, attachmentReader);
    m_avsConnectionManager->sendMessage(messageRequest);
    ASSERT_TRUE(messageRequest->waitFor(expectedStatus, timeout));
    ASSERT_TRUE(messageRequest->hasSendCompleted() || messageRequest->wasExceptionReceived());
}

void NetworkIntegrationTests::addDelay(const std::string& delayTime) {
    std::string str = "tc qdisc add dev " + network_interface + " root netem delay " + delayTime;
    int ret = system(str.c_str());
    ASSERT_TRUE(WIFEXITED(ret)) << "System call didn't terminate normally";
    ASSERT_EQ(0, WEXITSTATUS(ret)) << "Child process in system call exited abnormally";
}

void NetworkIntegrationTests::deleteDelay() {
    std::string str = "tc qdisc delete dev " + network_interface + " root";
    int ret = system(str.c_str());
    ASSERT_TRUE(WIFEXITED(ret)) << "System call didn't terminate normally";
    ASSERT_EQ(0, WEXITSTATUS(ret)) << "Child process in system call exited abnormally";
}

/**
 * Test if connection and disconnection can be established after delay is introduced.
 */
TEST_F(NetworkIntegrationTests, testConnectAfterSlowConnection) {
    addDelay(DELAY_TIME);
    connect();
    disconnect();
}

/**
 * Establish Connection, and introduce delay and check if @c connectionStatus remains CONNECTED.
 */
TEST_F(NetworkIntegrationTests, testConnectBeforeSlowConnection) {
    connect();
    addDelay(DELAY_TIME);
    disconnect();
}

/**
 * Establish connection, introduce delay and test if connect can be done again.
 */
TEST_F(NetworkIntegrationTests, testReConnectAfterDelay) {
    connect();
    addDelay(DELAY_TIME);
    disconnect();
    connect();
    disconnect();
}

/**
 * Establish connection, introduce a delay, send a message, check if the Status of MessageRequest
 * is SUCCESS.
 */
TEST_F(NetworkIntegrationTests, testSendEventAfterDelayPass) {
    connect();
    addDelay(DELAY_TIME);
    sendEvent(
        SYNCHRONIZE_STATE_JSON, MessageRequest::Status::SUCCESS, std::chrono::seconds(TIMEOUT_FOR_SEND_IN_SECONDS));
    disconnect();
}

/**
 * Establish connection, introduce a longer delay time of greater than 30 seconds, send a message,
 * the Status of MessageRequest will be TIMEDOUT.
 */
TEST_F(NetworkIntegrationTests, testSendEventAfterDelayFails) {
    connect();
    addDelay(LONG_DELAY_TIME);
    sendEvent(
        SYNCHRONIZE_STATE_JSON,
        MessageRequest::Status::TIMEDOUT,
        std::chrono::seconds(LONG_TIMEOUT_FOR_SEND_IN_SECONDS));
    disconnect();
}
}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (getuid()) {
        std::cerr << "You need to be root to run this test" << std::endl;
        return 1;
    }
    if (argc < 3) {
        std::cerr << "USAGE: " << std::string(argv[0]) << "<path_to_AlexaClientSDKConfig.json> <Network_Interface_Name>"
                  << std::endl;
        return 1;
    } else {
        alexaClientSDK::integration::test::g_configPath = std::string(argv[1]);
        alexaClientSDK::integration::test::network_interface = std::string(argv[2]);
        return RUN_ALL_TESTS();
    }
}
