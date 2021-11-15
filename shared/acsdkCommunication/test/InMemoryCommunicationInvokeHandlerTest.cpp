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

/// @file InMemoryCommunicationInvokeHandlerTest.cpp

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include <AVSCommon/Utils/Error/SuccessResult.h>
#include <acsdkCommunicationInterfaces/FunctionInvokerInterface.h>
#include "acsdkCommunication/InMemoryCommunicationInvokeHandler.h"

namespace alexaClientSDK {
namespace acsdkCommunication {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::utils::error;

class InMemoryCommunicationInvokeHandlerTest : public ::testing::Test {};

class TestFunction1 : public acsdkCommunicationInterfaces::FunctionInvokerInterface<std::string, int> {
public:
    std::string functionToBeInvoked(const std::string& name, int value) override {
        return "TestFunction1 " + name + " " + std::to_string(value);
    }
};
class TestFunction2 : public acsdkCommunicationInterfaces::FunctionInvokerInterface<std::string, int> {
public:
    std::string functionToBeInvoked(const std::string& name, int value) override {
        return "TestFunction2 " + name + " " + std::to_string(value);
    }
};

/**
 * Verify the registration
 */
TEST_F(InMemoryCommunicationInvokeHandlerTest, test_registerFunction) {
    InMemoryCommunicationInvokeHandler<std::string, int> handler;
    auto testFunction1 = std::make_shared<TestFunction1>(TestFunction1());
    auto testFunction2 = std::make_shared<TestFunction2>(TestFunction2());
    // registration should be successful
    ASSERT_TRUE(handler.registerFunction("test", testFunction1));
    // function with the same name should fail.
    ASSERT_FALSE(handler.registerFunction("test", testFunction2));
    // We shouldn't be able to register a nullptr function.
    ASSERT_FALSE(handler.registerFunction("test2", nullptr));

    // deregister to clean up.
    ASSERT_TRUE(handler.deregister("test", testFunction1));
}

/**
 * Verify deregistration
 */
TEST_F(InMemoryCommunicationInvokeHandlerTest, test_deregisterFunction) {
    InMemoryCommunicationInvokeHandler<std::string, int> handler;
    auto testFunction1 = std::make_shared<TestFunction1>(TestFunction1());
    auto testFunction2 = std::make_shared<TestFunction2>(TestFunction2());
    // register two functions
    ASSERT_TRUE(handler.registerFunction("test1", testFunction1));
    ASSERT_TRUE(handler.registerFunction("test2", testFunction2));
    // deregister a function
    ASSERT_TRUE(handler.deregister("test1", testFunction1));
    // try to deregister the same function again
    ASSERT_FALSE(handler.deregister("test1", testFunction1));
    // try to deregister a registered function with the wrong pointer
    ASSERT_FALSE(handler.deregister("test2", testFunction1));

    // deregister to clean up
    ASSERT_TRUE(handler.deregister("test2", testFunction2));
}
/**
 * Verify invoking the functions
 */
TEST_F(InMemoryCommunicationInvokeHandlerTest, test_invokeFunctions) {
    InMemoryCommunicationInvokeHandler<std::string, int> handler;
    const std::string name1 = "test1";
    const std::string name2 = "test2";
    int value1 = 1;
    int value2 = 2;
    std::string expectedReturnValue1 = "TestFunction1 " + name1 + " " + std::to_string(value1);
    std::string expectedReturnValue2 = "TestFunction2 " + name2 + " " + std::to_string(value2);

    auto testFunction1 = std::make_shared<TestFunction1>(TestFunction1());
    auto testFunction2 = std::make_shared<TestFunction2>(TestFunction2());
    // register two functions
    ASSERT_TRUE(handler.registerFunction("test1", testFunction1));
    ASSERT_TRUE(handler.registerFunction("test2", testFunction2));

    // invoke functions
    SuccessResult<std::string> returnValue1 = handler.invoke(name1, value1);
    SuccessResult<std::string> returnValue2 = handler.invoke(name2, value2);

    ASSERT_TRUE(returnValue1.isSucceeded());
    ASSERT_EQ(expectedReturnValue1, returnValue1.value());

    ASSERT_TRUE(returnValue2.isSucceeded());
    ASSERT_EQ(expectedReturnValue2, returnValue2.value());

    // unregistered function shouldn't be invoked.
    SuccessResult<std::string> returnValue3 = handler.invoke("test3", value1);
    ASSERT_FALSE(returnValue3.isSucceeded());

    // deregister to clean up
    ASSERT_TRUE(handler.deregister("test1", testFunction1));
    ASSERT_TRUE(handler.deregister("test2", testFunction2));
}

}  // namespace test
}  // namespace acsdkCommunication
}  // namespace alexaClientSDK
