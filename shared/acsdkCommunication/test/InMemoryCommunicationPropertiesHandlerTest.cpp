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

/// @file InMemoryCommunicationPropertiesHandlerTest.cpp

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include <acsdkCommunicationInterfaces/CommunicationPropertyChangeSubscriber.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertyValidatorInterface.h>

#include "acsdkCommunication/AlwaysTrueCommunicationValidator.h"
#include "acsdkCommunication/InMemoryCommunicationPropertiesHandler.h"

namespace alexaClientSDK {
namespace acsdkCommunication {
namespace test {

using namespace ::testing;

class InMemoryCommunicationPropertiesHandlerTest : public ::testing::Test {};

class TestSubscriber : public acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<int> {
public:
    MOCK_METHOD2(onCommunicationPropertyChange, void(const std::string&, int));
};
class FalseValidator : public acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<int> {
public:
    // always false validator.
    bool validateWriteRequest(const std::string& PropertyName, int newValue) {
        return false;
    }
};

/**
 * Verify the registration of a property
 */
TEST_F(InMemoryCommunicationPropertiesHandlerTest, test_registerProperty) {
    InMemoryCommunicationPropertiesHandler<int> handler;

    auto property1 = handler.registerProperty("test1", 1);
    ASSERT_NE(property1, nullptr);

    // try to reregister a property
    auto property2 = handler.registerProperty("test1", 2);
    ASSERT_EQ(property2, nullptr);

    // clean up deregister property
    handler.deregisterProperty("test1", property1);
}

/**
 * Verify write property
 */
TEST_F(InMemoryCommunicationPropertiesHandlerTest, test_writeProperty) {
    InMemoryCommunicationPropertiesHandler<int> handler;
    auto falseValidator = std::make_shared<FalseValidator>(FalseValidator());
    auto trueValidator =
        std::make_shared<AlwaysTrueCommunicationValidator<int>>(AlwaysTrueCommunicationValidator<int>());

    auto property1 = handler.registerProperty("test1", 1);
    ASSERT_NE(property1, nullptr);

    auto property2 = handler.registerProperty("test2", 2, trueValidator);
    ASSERT_NE(property2, nullptr);

    auto property3 = handler.registerProperty("test3", 3, falseValidator);
    ASSERT_NE(property3, nullptr);

    ASSERT_FALSE(handler.writeProperty("test1", 2));

    ASSERT_TRUE(handler.writeProperty("test2", 3));
    int value;
    handler.readProperty("test2", value);
    ASSERT_EQ(value, 3);

    ASSERT_FALSE(handler.writeProperty("test3", 4));

    handler.deregisterProperty("test1", property1);
    handler.deregisterProperty("test2", property2);
    handler.deregisterProperty("test3", property3);
}
/**
 * Validate write and subscribe
 */
TEST_F(InMemoryCommunicationPropertiesHandlerTest, test_subscribeWriteProperty) {
    InMemoryCommunicationPropertiesHandler<int> handler;
    auto trueValidator =
        std::make_shared<AlwaysTrueCommunicationValidator<int>>(AlwaysTrueCommunicationValidator<int>());
    std::condition_variable trigger;
    bool boolTrigger1 = false;
    bool boolTrigger2 = false;
    std::mutex mutex1;

    auto subscriber1 = std::make_shared<TestSubscriber>();
    auto subscriber2 = std::make_shared<TestSubscriber>();
    EXPECT_CALL(*subscriber1, onCommunicationPropertyChange(_, _)).WillOnce(InvokeWithoutArgs([&] {
        std::unique_lock<std::mutex> lock(mutex1);
        boolTrigger1 = true;
        trigger.notify_all();
    }));

    EXPECT_CALL(*subscriber2, onCommunicationPropertyChange(_, _)).WillOnce(InvokeWithoutArgs([&] {
        std::unique_lock<std::mutex> lock(mutex1);
        boolTrigger2 = true;
        trigger.notify_all();
    }));

    // subscribe prior to property being created.
    handler.subscribeToPropertyChangeEvent("test1", subscriber1);

    auto property1 = handler.registerProperty("test1", 1, trueValidator);
    ASSERT_NE(property1, nullptr);
    auto property2 = handler.registerProperty("test2", 2, trueValidator);
    ASSERT_NE(property2, nullptr);

    // subscribe after property has been created
    handler.subscribeToPropertyChangeEvent("test2", subscriber2);

    int newValue1 = 3;
    ASSERT_TRUE(handler.writeProperty("test1", newValue1));

    int value1;
    handler.readProperty("test1", value1);
    ASSERT_EQ(value1, newValue1);

    int newValue2 = 4;
    ASSERT_TRUE(handler.writeProperty("test2", newValue2));

    int value2;
    handler.readProperty("test2", value2);
    ASSERT_EQ(value2, newValue2);

    std::unique_lock<std::mutex> lock(mutex1);
    trigger.wait(lock, [&] { return boolTrigger1; });

    trigger.wait(lock, [&] { return boolTrigger2; });

    ASSERT_TRUE(handler.unsubscribeToPropertyChangeEvent("test1", subscriber1));
    ASSERT_TRUE(handler.unsubscribeToPropertyChangeEvent("test2", subscriber2));

    handler.deregisterProperty("test1", property1);
    handler.deregisterProperty("test2", property2);
}

}  // namespace test
}  // namespace acsdkCommunication
}  // namespace alexaClientSDK
