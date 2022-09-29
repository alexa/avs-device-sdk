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

#include <condition_variable>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdkInteractionModelInterfaces/InteractionModelRequestProcessingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/Endpoints/MockEndpointCapabilitiesRegistrar.h>

#include "acsdkInteractionModel/InteractionModelCapabilityAgent.h"
#include "acsdkInteractionModel/InteractionModelNotifier.h"

namespace alexaClientSDK {
namespace acsdkInteractionModel {
namespace test {

using namespace acsdkInteractionModelInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace ::testing;

const std::string TEST_DIALOG_REQUEST_AVS = "2";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": "2"
            }
        }
    })delim";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string CORRECT_REQUEST_PROCESSING_STARTED_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "RequestProcessingStarted",
                "messageId": "12345",
                "dialogRequestId": "3456"
            },
            "payload": {
            }
        }
    })delim";

/// An invalid NewDialogRequest directive with an incorrect name
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_1 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest1",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": "2"
            }
        }
    })delim";

/// An invalid NewDialogRequest directive with no payload
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_2 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                
            }
        }
    })delim";

/// An invalid NewDialogRequest with invalid dialogRequestID format
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_3 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": 2
            }
        }
    })delim";
/// An invalid NewDialogRequest with empty dialogRequestID format
static const std::string INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_4 = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "NewDialogRequest",
                "messageId": "12345"
            },
            "payload": {
                "dialogRequestId": ""
            }
        }
    })delim";

/// A sample RPS Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string RPS_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "RequestProcessingStarted",
                "messageId": "12345"
            },
            "payload": {
            }
        }
    })delim";

/// A sample RPC Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string RPC_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "InteractionModel",
                "name": "RequestProcessingCompleted",
                "messageId": "12345"
            },
            "payload": {
            }
        }
    })delim";

// Timeout to wait before indicating a test failed.
std::chrono::milliseconds TIMEOUT{500};

/// A wrapper for InteractionModelCapabilityAgent for easy testing
class InteractionModelCapabilityAgentWrapper : public InteractionModelCapabilityAgent {
public:
    static void handleDirectiveWrapper(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::shared_ptr<InteractionModelCapabilityAgent> capabilityAgent) {
        capabilityAgent->handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
    }
    static void preHandleDirectiveWrapper(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::shared_ptr<InteractionModelCapabilityAgent> capabilityAgent) {
        capabilityAgent->preHandleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
    }
};

/// Test harness for @c InteractionModelCapabilityAgent class.
class InteractionModelCapabilityAgentTest : public Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// The InteractionModelCapabilityAgent instance to be tested
    std::shared_ptr<InteractionModelCapabilityAgent> m_interactionModelCA;

    /// The InteractionModelNotifier that relays notifications to observers.
    std::shared_ptr<InteractionModelNotifierInterface> m_interactionModelNotifier;

    /// The mock @c EndpointCapabilitiesRegistrarInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>
        m_mockEndpointCapabilitiesRegistrar;

    /// The mock @c DirectiveSequencerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockDirectiveSequencer> m_mockDirectiveSequencer;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
};

class MockObserver : public InteractionModelRequestProcessingObserverInterface {
public:
    /// @name InteractionModelRequestProcessingObserverInterface Functions
    /// @{
    void onRequestProcessingStarted() override;
    void onRequestProcessingCompleted() override;
    /// @}

    /**
     * Waits for the onRequestProcessingStarted() function to be called.
     *
     * @return Whether or not onRequestProcessingStarted() was called before the timeout.
     */
    bool waitOnRPS();

    /**
     * Waits for the onRequestProcessingCompleted() function to be called.
     *
     * @return Whether or not onRequestProcessingCompleted() was called before the timeout.
     */
    bool waitOnRPC();

protected:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_rpcCalled;
    bool m_rpsCalled;
};

void MockObserver::onRequestProcessingStarted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rpsCalled = true;
    m_cond.notify_all();
}

void MockObserver::onRequestProcessingCompleted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rpcCalled = true;
    m_cond.notify_all();
}

bool MockObserver::waitOnRPS() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cond.wait_for(lock, TIMEOUT, [this] { return m_rpsCalled; });
}

bool MockObserver::waitOnRPC() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cond.wait_for(lock, TIMEOUT, [this] { return m_rpcCalled; });
}

void InteractionModelCapabilityAgentTest::SetUp() {
    m_mockDirectiveSequencer = std::make_shared<avsCommon::sdkInterfaces::test::MockDirectiveSequencer>();

    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();

    m_mockEndpointCapabilitiesRegistrar =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>>();
    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()));

    m_interactionModelNotifier = InteractionModelNotifier::createInteractionModelNotifierInterface();

    m_interactionModelCA = InteractionModelCapabilityAgent::create(
        m_mockDirectiveSequencer,
        m_mockExceptionEncounteredSender,
        m_interactionModelNotifier,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
            m_mockEndpointCapabilitiesRegistrar));

    ASSERT_NE(m_interactionModelCA, nullptr);
}

/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if directiveSequencer param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_createNoDirectiveSequencer) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(
        nullptr,
        m_mockExceptionEncounteredSender,
        m_interactionModelNotifier,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
            m_mockEndpointCapabilitiesRegistrar));

    ASSERT_EQ(m_interactionModelCA, nullptr);
}

/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if exceptionHandler param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_createNoExceptionHandler) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(
        m_mockDirectiveSequencer,
        nullptr,
        m_interactionModelNotifier,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
            m_mockEndpointCapabilitiesRegistrar));

    ASSERT_EQ(m_interactionModelCA, nullptr);
}

/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if notifier param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_createNoNotifier) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(
        m_mockDirectiveSequencer,
        m_mockExceptionEncounteredSender,
        nullptr,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
            m_mockEndpointCapabilitiesRegistrar));

    ASSERT_EQ(m_interactionModelCA, nullptr);
}

/**
 * Test to verify the @c InteractionModelCapabilityAgent can not be created if capabilitiesRegistrar param is null.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_createNoEndpointCapabilitiesRegistrar) {
    m_interactionModelCA = InteractionModelCapabilityAgent::create(
        m_mockDirectiveSequencer, m_mockExceptionEncounteredSender, m_interactionModelNotifier, nullptr);

    ASSERT_EQ(m_interactionModelCA, nullptr);
}

/**
 * Test to verify if a valid NewDialogRequest directive will set the dialogRequestID in the directive sequencer.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_processNewDialogRequestID) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    m_interactionModelCA->handleDirectiveImmediately(directive);
    ASSERT_EQ(TEST_DIALOG_REQUEST_AVS, m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if a valid NewDialogRequest directive will set the dialogRequestID in the directive sequencer
 * in preHandle hook.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_preHandledNewDialogRequestID) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);
    std::shared_ptr<CapabilityAgent> agent = std::dynamic_pointer_cast<CapabilityAgent>(m_interactionModelCA);

    agent->preHandleDirective(directive, nullptr);
    ASSERT_EQ(TEST_DIALOG_REQUEST_AVS, m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if preHandle interface will NOT process directive having dialogRequestID.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_preHandledRequestProcessingStarted) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(CORRECT_REQUEST_PROCESSING_STARTED_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    InteractionModelCapabilityAgentWrapper::preHandleDirectiveWrapper(directive, m_interactionModelCA);
    ASSERT_EQ("", m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if preHandle interface will ignore directives with dialogRequestId.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_preHandledNullNewDialogRequestID) {
    std::shared_ptr<CapabilityAgent> agent = std::dynamic_pointer_cast<CapabilityAgent>(m_interactionModelCA);

    agent->preHandleDirective(nullptr, nullptr);
    ASSERT_EQ("", m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if a valid NewDialogRequest directive with empty dialogRequestId
 * will not be handled in handleDirective hook.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_handledNewDialogRequestID) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(CORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    InteractionModelCapabilityAgentWrapper::handleDirectiveWrapper(directive, m_interactionModelCA);
    ASSERT_EQ("", m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if interface will ignore null directives
 */
TEST_F(InteractionModelCapabilityAgentTest, test_processNullDirective) {
    m_interactionModelCA->handleDirectiveImmediately(nullptr);
    ASSERT_EQ("", m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test to verify if interface will send exceptions when the directive received is invalid
 */
TEST_F(InteractionModelCapabilityAgentTest, test_processInvalidDirective) {
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_1, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_2, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive3 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_3, nullptr, "").first;
    std::shared_ptr<AVSDirective> directive4 =
        AVSDirective::create(INCORRECT_NEW_DIALOG_REQUEST_DIRECTIVE_JSON_STRING_4, nullptr, "").first;

    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _)).Times(4);
    m_interactionModelCA->handleDirectiveImmediately(directive1);
    m_interactionModelCA->handleDirectiveImmediately(directive2);
    m_interactionModelCA->handleDirectiveImmediately(directive3);
    m_interactionModelCA->handleDirectiveImmediately(directive4);
    ASSERT_EQ("", m_mockDirectiveSequencer->getDialogRequestId());
}

/**
 * Test add an observer succeeds and receives RPS directives.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_addObserverRPS) {
    std::shared_ptr<MockObserver> observer = std::make_shared<MockObserver>();
    m_interactionModelNotifier->addObserver(observer);

    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(RPS_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    m_interactionModelCA->handleDirectiveImmediately(directive);
    EXPECT_TRUE(observer->waitOnRPS());
}

/**
 * Test add an observer succeeds and receives RPC directives.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_addObserverRPC) {
    std::shared_ptr<MockObserver> observer = std::make_shared<MockObserver>();
    m_interactionModelNotifier->addObserver(observer);

    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(RPC_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    m_interactionModelCA->handleDirectiveImmediately(directive);
    EXPECT_TRUE(observer->waitOnRPC());
}

/**
 * Test removing an observer results in no callbacks.
 */
TEST_F(InteractionModelCapabilityAgentTest, test_removeObserver) {
    std::shared_ptr<MockObserver> observer = std::make_shared<MockObserver>();
    m_interactionModelNotifier->addObserver(observer);
    m_interactionModelNotifier->removeObserver(observer);

    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(RPC_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    m_interactionModelCA->handleDirectiveImmediately(directive);
    EXPECT_FALSE(observer->waitOnRPC());
}

}  // namespace test
}  // namespace acsdkInteractionModel
}  // namespace alexaClientSDK
