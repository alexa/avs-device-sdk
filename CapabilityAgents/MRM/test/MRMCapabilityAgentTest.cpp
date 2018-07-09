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

#include <iterator>
#include <memory>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockSpeakerManager.h>
#include <AVSCommon/SDKInterfaces/MockUserInactivityMonitor.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>

#include "MRM/MRMCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace mrm {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace ::testing;

/// Test version string which our dummy MRMHandler will return.
static const std::string TEST_MRM_HANDLER_VERSION_STRING = "test_version_string";

/// A timeout for things which we expect to occur (so it's long enough for reasonable latency)
static const auto WAIT_FOR_INVOCATION_LONG_TIMEOUT = std::chrono::milliseconds{100};

/// A timeout for things which we do not expect to occur (so, we will expect this duration to elapse).
static const auto WAIT_FOR_INVOCATION_SHORT_TIMEOUT = std::chrono::milliseconds{5};

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string TEST_DIRECTIVE_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "MRM",
                "name": "TestDirective",
                "messageId": "12345"
            },
            "payload": {

            }
        }
    })delim";

/**
 * A utility class to simplify capturing that the invocation of an event has occurred.  This class expects that
 * two threads are involved - thread A waits for the invocation, while thread B performs the invocation.
 */
class SynchronizedInvocation {
public:
    /**
     * Constructor.
     */
    SynchronizedInvocation() : m_hasBeenInvoked{false} {
    }

    /**
     * Record that the invocation has occurred.  Calling this function will wake any waiting threads.
     */
    void invoke() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_hasBeenInvoked = true;
        m_cv.notify_all();
    }

    /**
     * Wait for an invocation to occur.  If it has already occurred, this function will immediately return true.
     * If the invocation did occur, then this function resets its tracking data, allowing this object to be used
     * repeatedly.
     *
     * @param timeout How long the caller wishes to wait for the invocation to occur.
     * @return Whether the invocation occurred within the timeout, or if it has already occurred before the call.
     */
    bool wait(const std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_cv.wait_for(lock, timeout, [this]() { return m_hasBeenInvoked; })) {
            m_hasBeenInvoked = false;
            return true;
        }
        return false;
    }

private:
    /// The mutex controlling access to the boolean.
    std::mutex m_mutex;
    /// The condition variable which may be waited upon.
    std::condition_variable m_cv;
    /// A boolean variable to capture an event occurring.
    bool m_hasBeenInvoked;
};

/**
 * WaitableExceptionEncounteredSender is a mock of the @c ExceptionEncounteredSenderInterface and allows tests
 * to wait for invocations upon those interfaces and inspect the parameters of those invocations.
 */
class WaitableExceptionEncounteredSender : public ExceptionEncounteredSenderInterface {
public:
    void sendExceptionEncountered(
        const std::string& unparsedDirective,
        avsCommon::avs::ExceptionErrorType error,
        const std::string& message) override {
        m_invocation.invoke();
    }

    /**
     * Allows the caller to wait until an exception has been sent, up to a maximum timeout.
     * Also returns true if an exception has been previously sent which was not waited upon.
     *
     * @return Whether the exception was sent within the specified timeout, or if an exception has been previously sent,
     * which was not waited upon.
     */
    bool wait(const std::chrono::milliseconds timeout) {
        return m_invocation.wait(timeout);
    }

private:
    /// Our invocation object to track when an exception has been sent.
    SynchronizedInvocation m_invocation;
};

/**
 * Class with which to mock a connection to AVS.
 */
class MockMRMHandler : public MRMHandlerInterface {
public:
    /**
     * Constructor.
     */
    MockMRMHandler() : MRMHandlerInterface{"MockMRMHandler"} {
    }

    MOCK_CONST_METHOD0(getVersionString, std::string());
    MOCK_METHOD1(handleDirective, bool(std::shared_ptr<AVSDirective> directive));
    MOCK_METHOD0(doShutdown, void());

    /**
     * overridden function, minus the explicit override, since gtest does not use override.
     */
    void onSpeakerSettingsChanged(const SpeakerInterface::Type& type) {
        m_lastSpeakerType = type;
        m_speakerSettingUpdatedInvocation.invoke();
    }

    /**
     * overridden function, minus the explicit override, since gtest does not use override.
     */
    void onUserInactivityReportSent() {
        m_userInactivityInvocation.invoke();
    }

    /**
     * Function to wait for a speaker setting of a particular type to change.  Will return true if a speaker of the
     * desired type did change within the timeout, or if a non-waited-upon change occurred before this call was made.
     *
     * @param expectedType The type of the speaker we are waiting for.
     * @param timeout How long we wish to wait for the change to occur.
     * @return Whether the change occurred within the timeout, or if a non-waited-upon change occurred before this call.
     */
    bool waitForSpeakerSettingChanged(
        const SpeakerInterface::Type& expectedType,
        const std::chrono::milliseconds timeout) {
        if (m_speakerSettingUpdatedInvocation.wait(timeout)) {
            return expectedType == m_lastSpeakerType;
        }
        return false;
    }

    /**
     * Function to wait for a System.UserInactivityReport to be sent.  Will return true if an exception was sent
     * within the timeout, or if a non-waited-upon report occurred before this call was made.
     *
     * @param timeout How long we wish to wait for the report to occur.
     * @return Whether the report occurred within the timeout, or if a non-waited-upon report occurred before this call.
     */
    bool waitForUserInactivityReport(const std::chrono::milliseconds timeout) {
        return m_userInactivityInvocation.wait(timeout);
    }

private:
    /// Our invocation object to track the sending of a System.UserInactivityReport.
    SynchronizedInvocation m_userInactivityInvocation;
    /// Our invocation object to track the changing of a speaker type.
    SynchronizedInvocation m_speakerSettingUpdatedInvocation;
    /// A tracking variable for the most recently changed speaker type.
    SpeakerInterface::Type m_lastSpeakerType;
};

/// Test harness for @c MRMCapabilityAgent class.
class MRMCapabilityAgentTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    /// Our MRM CA.
    std::shared_ptr<MRMCapabilityAgent> m_mrmCA;
    /// A pointer to a dummy MRMHandler.  Since the CA takes the Handler by unique_ptr, we will keep the raw pointer.
    MockMRMHandler* m_mockMRMHandlerPtr;
    /// A mock speaker manager.
    std::shared_ptr<MockSpeakerManager> m_mockSpeakerManager;
    /// A mock user inactivity monitor.
    std::shared_ptr<MockUserInactivityMonitor> m_mockUserInactivityMonitor;
    /// A waitable exception sender.
    std::shared_ptr<WaitableExceptionEncounteredSender> m_exceptionSender;
};

void MRMCapabilityAgentTest::SetUp() {
    // Create our main objects.
    auto mrmHandler = std::unique_ptr<MRMHandlerInterface>(new MockMRMHandler());
    m_mockMRMHandlerPtr = static_cast<MockMRMHandler*>(mrmHandler.get());

    m_mockSpeakerManager = std::make_shared<MockSpeakerManager>();
    m_mockUserInactivityMonitor = std::make_shared<MockUserInactivityMonitor>();
    m_exceptionSender = std::make_shared<WaitableExceptionEncounteredSender>();

    // Instantiate the capability agent.
    EXPECT_CALL(*m_mockSpeakerManager, addSpeakerManagerObserver(_)).Times(1);
    EXPECT_CALL(*m_mockUserInactivityMonitor, addObserver(_)).Times(1);

    m_mrmCA = MRMCapabilityAgent::create(
        std::move(mrmHandler), m_mockSpeakerManager, m_mockUserInactivityMonitor, m_exceptionSender);

    ASSERT_NE(m_mrmCA, nullptr);
}

void MRMCapabilityAgentTest::TearDown() {
    EXPECT_CALL(*m_mockMRMHandlerPtr, doShutdown()).Times(1);
    EXPECT_CALL(*m_mockSpeakerManager, removeSpeakerManagerObserver(_)).Times(1);
    EXPECT_CALL(*m_mockUserInactivityMonitor, removeObserver(_)).Times(1);

    m_mrmCA->shutdown();
}

/**
 * Test to verify the @c create function of @c MRMCapabilityAgent class.
 */
TEST_F(MRMCapabilityAgentTest, createTest) {
    /// A dummy MRMHandler.
    auto mrmHandler = std::unique_ptr<MRMHandlerInterface>(new MockMRMHandler());

    /// Test all the bad cases.  The good case is already tested in SetUp.
    ASSERT_EQ(
        nullptr,
        MRMCapabilityAgent::create(nullptr, m_mockSpeakerManager, m_mockUserInactivityMonitor, m_exceptionSender));
    ASSERT_EQ(
        nullptr,
        MRMCapabilityAgent::create(std::move(mrmHandler), nullptr, m_mockUserInactivityMonitor, m_exceptionSender));
    ASSERT_EQ(
        nullptr, MRMCapabilityAgent::create(std::move(mrmHandler), m_mockSpeakerManager, nullptr, m_exceptionSender));
    ASSERT_EQ(
        nullptr,
        MRMCapabilityAgent::create(std::move(mrmHandler), m_mockSpeakerManager, m_mockUserInactivityMonitor, nullptr));
}

/**
 * Test to verify the @c getConfiguration function of @c MRMCapabilityAgent class.
 */
TEST_F(MRMCapabilityAgentTest, getConfigurationTest) {
    auto config = m_mrmCA->getConfiguration();
    ASSERT_NE(true, config.empty());
}

/**
 * Test to verify the @c getVersionString function of @c MRMCapabilityAgent class.
 */
TEST_F(MRMCapabilityAgentTest, getVersionStringTest) {
    EXPECT_CALL(*m_mockMRMHandlerPtr, getVersionString()).WillOnce(Return(TEST_MRM_HANDLER_VERSION_STRING));
    std::string versionString = m_mrmCA->getVersionString();
    ASSERT_NE(true, versionString.empty());
}

/**
 * Test to verify the @c handleDirective function of @c MRMHandler class, invoked by the @c MRMCapabilityAgent class.
 */
TEST_F(MRMCapabilityAgentTest, handleMRMDirectiveTest) {
    // Create a dummy AVSDirective.
    auto directivePair = AVSDirective::create(TEST_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    // Test that the MRMHandler will receive the Directive and fail to handle it.
    EXPECT_CALL(*m_mockMRMHandlerPtr, handleDirective(_)).WillOnce(Return(false));
    m_mrmCA->handleDirectiveImmediately(directive);
    ASSERT_TRUE(m_exceptionSender->wait(WAIT_FOR_INVOCATION_LONG_TIMEOUT));

    // Test that the MRMHandler will receive the Directive and successfully handle it.
    EXPECT_CALL(*m_mockMRMHandlerPtr, handleDirective(_)).WillOnce(Return(true));
    m_mrmCA->handleDirectiveImmediately(directive);
    ASSERT_FALSE(m_exceptionSender->wait(WAIT_FOR_INVOCATION_SHORT_TIMEOUT));
}

/**
 * Test to verify the @c onSpeakerSettingsChanged function of @c MRMHandler class, invoked by the @c MRMCapabilityAgent
 * class.
 */
TEST_F(MRMCapabilityAgentTest, onSpeakerSettingsChangedTest) {
    SpeakerInterface::SpeakerSettings dummySpeakerSettings;

    // Test that the AVS_ALERTS_VOLUME option works.
    m_mrmCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::DIRECTIVE,
        SpeakerInterface::Type::AVS_ALERTS_VOLUME,
        dummySpeakerSettings);
    ASSERT_TRUE(m_mockMRMHandlerPtr->waitForSpeakerSettingChanged(
        SpeakerInterface::Type::AVS_ALERTS_VOLUME, WAIT_FOR_INVOCATION_LONG_TIMEOUT));

    // Test that the AVS_SPEAKER_VOLUME option works.
    m_mrmCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::DIRECTIVE,
        SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        dummySpeakerSettings);
    ASSERT_TRUE(m_mockMRMHandlerPtr->waitForSpeakerSettingChanged(
        SpeakerInterface::Type::AVS_SPEAKER_VOLUME, WAIT_FOR_INVOCATION_LONG_TIMEOUT));
}

/**
 * Test to verify the @c waitForUserInactivityReport function of @c MRMHandler class, invoked by the
 * @c MRMCapabilityAgent class.
 */
TEST_F(MRMCapabilityAgentTest, onUserInactivityReportTest) {
    // Verify that our Inactivity Report is sent by MRMHandler when invoked by the MRM CA.
    m_mrmCA->onUserInactivityReportSent();
    ASSERT_TRUE(m_mockMRMHandlerPtr->waitForUserInactivityReport(WAIT_FOR_INVOCATION_LONG_TIMEOUT));

    // Verify that the Inactivity Report is only sent once.
    ASSERT_FALSE(m_mockMRMHandlerPtr->waitForUserInactivityReport(WAIT_FOR_INVOCATION_SHORT_TIMEOUT));
}

}  // namespace test
}  // namespace mrm
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 1) {
        std::cerr << "USAGE: " << std::string(argv[0]) << std::endl;
        return 1;
    }
    return RUN_ALL_TESTS();
}