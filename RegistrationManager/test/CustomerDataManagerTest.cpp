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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "RegistrationManager/CustomerDataHandler.h"
#include "RegistrationManager/CustomerDataManager.h"

namespace alexaClientSDK {
namespace registrationManager {
namespace test {

class MockCustomerDataHandler : public CustomerDataHandler {
public:
    MockCustomerDataHandler(std::shared_ptr<CustomerDataManager> dataManager) : CustomerDataHandler(dataManager) {
    }
    MOCK_METHOD0(clearData, void());
};

class CustomerDataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_dataManager = std::make_shared<CustomerDataManager>();
    }

    void TearDown() override {
        m_dataManager.reset();
    }

    std::shared_ptr<CustomerDataManager> m_dataManager;
};

TEST_F(CustomerDataManagerTest, testEmptyManager) {
    m_dataManager->clearData();
}

/**
 * Test that all data handlers are cleared.
 */
TEST_F(CustomerDataManagerTest, testClearData) {
    MockCustomerDataHandler handler1{m_dataManager};
    MockCustomerDataHandler handler2{m_dataManager};
    EXPECT_CALL(handler1, clearData()).Times(1);
    EXPECT_CALL(handler2, clearData()).Times(1);
    m_dataManager->clearData();
}

/**
 * Test that removing a data handler does not leave any dangling reference inside @c CustomerDataManager.
 */
TEST_F(CustomerDataManagerTest, testClearDataAfterHandlerDeletion) {
    {
        // CustomerDataHandler will register and deregister with CustomerDataManager during ctor and dtor, respectively.
        MockCustomerDataHandler handler1{m_dataManager};
        EXPECT_CALL(handler1, clearData()).Times(0);
    }
    MockCustomerDataHandler handler2{m_dataManager};
    EXPECT_CALL(handler2, clearData()).Times(1);
    m_dataManager->clearData();
}

}  // namespace test
}  // namespace registrationManager
}  // namespace alexaClientSDK
