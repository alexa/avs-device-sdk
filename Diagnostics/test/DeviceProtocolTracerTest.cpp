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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Diagnostics/DeviceProtocolTracer.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace diagnostics {
namespace test {

using namespace avsCommon::utils::json::jsonUtils;

class DeviceProtocolTracerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// The @c DeviceProtocolTracer to test.
    std::shared_ptr<DeviceProtocolTracer> m_deviceProtocolTracer;
};

void DeviceProtocolTracerTest::SetUp() {
    m_deviceProtocolTracer = DeviceProtocolTracer::create();
}

void DeviceProtocolTracerTest::TearDown() {
    m_deviceProtocolTracer->setProtocolTraceFlag(false);
    m_deviceProtocolTracer->clearTracedMessages();
}

/**
 * Test if protocol tracing is disabled by default.
 */
TEST_F(DeviceProtocolTracerTest, test_ifProtocolTracingIsDisabledByDefault) {
    m_deviceProtocolTracer->setMaxMessages(100);

    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->traceEvent("Event1");

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[]");
}

/**
 * Test if the protocol tracer does not return the protocol trace when it's disabled.
 */
TEST_F(DeviceProtocolTracerTest, test_protocolTraceWithTraceFlagDisabled) {
    m_deviceProtocolTracer->setProtocolTraceFlag(false);
    m_deviceProtocolTracer->setMaxMessages(100);

    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->traceEvent("Event1");

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[]");
}

/**
 * Test if the protocol tracer returns the protocol trace when it's enabled.
 */
TEST_F(DeviceProtocolTracerTest, test_protocolTraceWithTraceFlagEnabled) {
    m_deviceProtocolTracer->setProtocolTraceFlag(true);
    m_deviceProtocolTracer->setMaxMessages(100);

    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->receive("contextId2", "Directive2");
    m_deviceProtocolTracer->traceEvent("Event1");

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[Directive1,Directive2,Event1]");
}

/**
 * Test if protocol tracing only traces DEFAULT_MAX_MESSAGES (1) by default.
 */
TEST_F(DeviceProtocolTracerTest, test_ifProtocolTracingTracesOneMessageByDefault) {
    m_deviceProtocolTracer->setProtocolTraceFlag(true);

    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->receive("contextId1", "Directive2");
    m_deviceProtocolTracer->traceEvent("Event1");

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[Directive1]");
}

/**
 * Test if clearTracedMessages clears the traced message list.
 */
TEST_F(DeviceProtocolTracerTest, test_clearTracedMessages) {
    m_deviceProtocolTracer->setProtocolTraceFlag(true);
    m_deviceProtocolTracer->setMaxMessages(100);

    m_deviceProtocolTracer->receive("contextId1", "Directive1");
    m_deviceProtocolTracer->receive("contextId2", "Directive2");
    m_deviceProtocolTracer->traceEvent("Event1");

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[Directive1,Directive2,Event1]");

    m_deviceProtocolTracer->clearTracedMessages();

    ASSERT_EQ(m_deviceProtocolTracer->getProtocolTrace(), "[]");
}

/**
 * Test if the protocol tracer abides by the maxMessages configuration.
 */
TEST_F(DeviceProtocolTracerTest, test_maxTracedMessageLimit) {
    m_deviceProtocolTracer->setProtocolTraceFlag(true);
    m_deviceProtocolTracer->setMaxMessages(100);

    for (int i = 1; i <= 100; ++i) {
        m_deviceProtocolTracer->receive("contextId", "Directive" + std::to_string(i));
        m_deviceProtocolTracer->traceEvent("Event" + std::to_string(i));
    }

    auto messageListJsonString = m_deviceProtocolTracer->getProtocolTrace();

    // Make sure the resulting string does not contain unexpected directives and events
    for (int i = 1; i <= 50; ++i) {
        ASSERT_TRUE(messageListJsonString.find("Directive" + std::to_string(i)) != std::string::npos);
        ASSERT_TRUE(messageListJsonString.find("Event" + std::to_string(i)) != std::string::npos);
    }

    for (int i = 51; i <= 100; ++i) {
        ASSERT_TRUE(messageListJsonString.find("Directive" + std::to_string(i)) == std::string::npos);
        ASSERT_TRUE(messageListJsonString.find("Event" + std::to_string(i)) == std::string::npos);
    }
}

/**
 * Test setMaxMessages and getMaxMessages work.
 */
TEST_F(DeviceProtocolTracerTest, test_maxMessagesGettersSetters) {
    const unsigned int expectedVal = m_deviceProtocolTracer->getMaxMessages() + 1;
    ASSERT_TRUE(m_deviceProtocolTracer->setMaxMessages(expectedVal));
    ASSERT_EQ(expectedVal, m_deviceProtocolTracer->getMaxMessages());
}

/**
 * Test setMaxMessages to a smaller amount than currently stored messages fails.
 */
TEST_F(DeviceProtocolTracerTest, test_setMaxMessagesFailsIfSmallerThanStoredMessages) {
    const unsigned int numMessages = 10;
    ASSERT_TRUE(m_deviceProtocolTracer->setMaxMessages(numMessages));
    m_deviceProtocolTracer->setProtocolTraceFlag(true);

    for (unsigned int i = 0; i < numMessages; ++i) {
        m_deviceProtocolTracer->receive("contextId", "Directive" + std::to_string(i));
        m_deviceProtocolTracer->traceEvent("Event" + std::to_string(i));
    }

    ASSERT_FALSE(m_deviceProtocolTracer->setMaxMessages(numMessages - 1));
    ASSERT_EQ(numMessages, m_deviceProtocolTracer->getMaxMessages());
}

}  // namespace test
}  // namespace diagnostics
}  // namespace alexaClientSDK
