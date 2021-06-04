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

#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include "acsdkDeviceSetup/DeviceSetup.h"
#include "acsdkDeviceSetup/DeviceSetupMessageRequest.h"

namespace alexaClientSDK {
namespace acsdkDeviceSetup {
namespace test {

using namespace avsCommon::avs;
using namespace acsdkDeviceSetupInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace rapidjson;
using namespace ::testing;

/// The namespace for Device Setup.
static const std::string DEVICE_SETUP_NAMESPACE{"DeviceSetup"};

/// The SetupCompleted event.
static const std::string SETUP_COMPLETED_EVENT{"SetupCompleted"};

/// A long timeout to ensure that an event does occur.
static std::chrono::seconds TIMEOUT{5};

class DeviceSetupTest : public ::testing::Test {
public:
    /// SetUp before each test.
    void SetUp();

protected:
    /// Mock message sender to set expectations against events.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /// A pointer to an instance of the DeviceSetup that will be instantiated per test.
    std::shared_ptr<DeviceSetup> m_deviceSetup;
};

void DeviceSetupTest::SetUp() {
    m_mockMessageSender = std::make_shared<MockMessageSender>();
    m_deviceSetup = DeviceSetup::create(m_mockMessageSender);
}

/**
 * Custom matcher to check an event with the correct assistedSetup value is sent.
 * Returns true if the event matching that criteria is sent.
 *
 * @param expectedNamespace The expected namespace of the event.
 * @param expectedName The expected name of the event.
 * @param assistedSetup The expected assistedSetup of the event.
 * @return true if an event matching the criteria is sent, false otherwise.
 */
MATCHER_P3(
    EventNamed,
    /* std::string */ expectedNameSpace,
    /* std::string */ expectedName,
    /* deviceSetup::AssistedSetup */ expectedAssistedSetup,
    "") {
    // Throw obvious compile error if not a MessageRequest.
    std::shared_ptr<MessageRequest> request = arg;

    if (!request) {
        return false;
    }

    rapidjson::Document document;
    ParseResult result = document.Parse(request->getJsonContent());

    if (!result) {
        return false;
    }

    rapidjson::Value::ConstMemberIterator eventIt;
    rapidjson::Value::ConstMemberIterator headerIt;
    rapidjson::Value::ConstMemberIterator payloadIt;
    if (!json::jsonUtils::findNode(document, "event", &eventIt) ||
        !json::jsonUtils::findNode(eventIt->value, "header", &headerIt) ||
        !json::jsonUtils::findNode(eventIt->value, "payload", &payloadIt)) {
        return false;
    }

    std::string name;
    std::string nameSpace;

    if (!json::jsonUtils::retrieveValue(headerIt->value, "name", &name) ||
        !json::jsonUtils::retrieveValue(headerIt->value, "namespace", &nameSpace) || nameSpace != expectedNameSpace ||
        name != expectedName) {
        return false;
    }

    std::string assistedSetup;

    if (!json::jsonUtils::retrieveValue(payloadIt->value, "assistedSetup", &assistedSetup) ||
        assistedSetup != assistedSetupToString(expectedAssistedSetup)) {
        return false;
    }

    return true;
}

/// Tests the constructor with nullptr as arguments.
TEST_F(DeviceSetupTest, constructorNullptr) {
    ASSERT_THAT(DeviceSetup::create(nullptr), IsNull());
}

/// Tests that sendDeviceSetupComplete sends an event.
TEST_F(DeviceSetupTest, sendDeviceSetupComplete) {
    EXPECT_CALL(
        *m_mockMessageSender,
        sendMessage(EventNamed(DEVICE_SETUP_NAMESPACE, SETUP_COMPLETED_EVENT, AssistedSetup::NONE)))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
        }));

    auto futureResult = m_deviceSetup->sendDeviceSetupComplete(AssistedSetup::NONE);
    ASSERT_TRUE(futureResult.valid());
    ASSERT_THAT(futureResult.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_TRUE(futureResult.get());
}

/// Tests that if sending an event fails, sendDevicesSetupComplete returns false.
TEST_F(DeviceSetupTest, sendDevicesSetupCompleteFails) {
    EXPECT_CALL(
        *m_mockMessageSender,
        sendMessage(EventNamed(DEVICE_SETUP_NAMESPACE, SETUP_COMPLETED_EVENT, AssistedSetup::NONE)))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        }));

    auto futureResult = m_deviceSetup->sendDeviceSetupComplete(AssistedSetup::NONE);
    ASSERT_TRUE(futureResult.valid());
    ASSERT_THAT(futureResult.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_FALSE(futureResult.get());
}

/// Tests that receiving an exception upon sendDevicesSetupComplete.
TEST_F(DeviceSetupTest, sendDevicesSetupCompleteException) {
    EXPECT_CALL(
        *m_mockMessageSender,
        sendMessage(EventNamed(DEVICE_SETUP_NAMESPACE, SETUP_COMPLETED_EVENT, AssistedSetup::NONE)))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->exceptionReceived("Exception");
            request->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        }));

    auto futureResult = m_deviceSetup->sendDeviceSetupComplete(AssistedSetup::NONE);
    ASSERT_TRUE(futureResult.valid());
    ASSERT_THAT(futureResult.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_FALSE(futureResult.get());
}

/// Test sending multiple requests and distinguish between them.
TEST_F(DeviceSetupTest, sendDevicesSetupCompleteMultiple) {
    EXPECT_CALL(
        *m_mockMessageSender,
        sendMessage(EventNamed(DEVICE_SETUP_NAMESPACE, SETUP_COMPLETED_EVENT, AssistedSetup::NONE)))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
        }));

    EXPECT_CALL(
        *m_mockMessageSender,
        sendMessage(
            EventNamed(DEVICE_SETUP_NAMESPACE, SETUP_COMPLETED_EVENT, AssistedSetup::ALEXA_COMPANION_APPLICATION)))
        .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
            request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
        }));

    auto futureFail = m_deviceSetup->sendDeviceSetupComplete(AssistedSetup::NONE);
    auto futureSucceed = m_deviceSetup->sendDeviceSetupComplete(AssistedSetup::ALEXA_COMPANION_APPLICATION);

    ASSERT_TRUE(futureSucceed.valid());
    ASSERT_THAT(futureSucceed.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_TRUE(futureSucceed.get());

    ASSERT_TRUE(futureFail.valid());
    ASSERT_THAT(futureFail.wait_for(TIMEOUT), std::future_status::ready);
    ASSERT_FALSE(futureFail.get());
}

}  // namespace test
}  // namespace acsdkDeviceSetup
}  // namespace alexaClientSDK
