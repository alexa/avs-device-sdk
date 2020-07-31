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

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockPostConnectObserver.h"
#include "MockPostConnectOperation.h"
#include <ACL/Transport/PostConnectSequencer.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace ::testing;

/// A short delay used in tests.
static const auto SHORT_DELAY = std::chrono::seconds(1);

/**
 * Test harness for @c PostConnectSequencer class.
 */
class PostConnectSequencerTest : public Test {
public:
    /// initialization for tests.
    void SetUp() override;

protected:
    /// The mock @c PostConnectObserverInterface.
    std::shared_ptr<MockPostConnectObserver> m_mockPostConnectObserver;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;
};

void PostConnectSequencerTest::SetUp() {
    m_mockPostConnectObserver = std::make_shared<NiceMock<MockPostConnectObserver>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
}

/**
 * Check if the PostConnectOperationsSet sequences operations based on priority.
 */
TEST_F(PostConnectSequencerTest, test_postConnectOperationsSet) {
    auto operation1 = std::make_shared<NiceMock<MockPostConnectOperation>>();
    auto operation2 = std::make_shared<NiceMock<MockPostConnectOperation>>();
    auto operation3 = std::make_shared<NiceMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(3));
    EXPECT_CALL(*operation2, getOperationPriority()).WillRepeatedly(Return(2));
    EXPECT_CALL(*operation3, getOperationPriority()).WillRepeatedly(Return(1));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);
    operationsSet.insert(operation2);
    operationsSet.insert(operation3);

    auto it = operationsSet.begin();
    ASSERT_EQ(*it, operation3);
    it++;
    ASSERT_EQ(*it, operation2);
    it++;
    ASSERT_EQ(*it, operation1);
}

/**
 * Check if the PostConnectOperationsSet keeps only one instance with the same priority.
 */
TEST_F(PostConnectSequencerTest, test_postConnectOperationsSetRemovesDuplicates) {
    auto operation1 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation2 = std::make_shared<StrictMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(3));
    EXPECT_CALL(*operation2, getOperationPriority()).WillRepeatedly(Return(3));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);
    operationsSet.insert(operation2);

    ASSERT_EQ(operationsSet.size(), 1U);
}

/**
 * Check if all PostConnectOperations are executed in sequence and the PostConnectObserver will be notified.
 */
TEST_F(PostConnectSequencerTest, test_happyPathAndPostConnectObserverGetsNotified) {
    auto operation1 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation2 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation3 = std::make_shared<StrictMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(3));
    EXPECT_CALL(*operation2, getOperationPriority()).WillRepeatedly(Return(2));
    EXPECT_CALL(*operation3, getOperationPriority()).WillRepeatedly(Return(1));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);
    operationsSet.insert(operation2);
    operationsSet.insert(operation3);

    auto postConnectSequencer = PostConnectSequencer::create(operationsSet);
    ASSERT_NE(postConnectSequencer, nullptr);

    {
        InSequence s;
        EXPECT_CALL(*operation3, performOperation(_)).WillOnce(Return(true));
        EXPECT_CALL(*operation2, performOperation(_)).WillOnce(Return(true));
        EXPECT_CALL(*operation1, performOperation(_)).WillOnce(Return(true));
    }

    PromiseFuturePair<bool> promiseFuturePair;
    EXPECT_CALL(*m_mockPostConnectObserver, onPostConnected()).WillOnce(Invoke([&promiseFuturePair] {
        promiseFuturePair.setValue(true);
    }));

    postConnectSequencer->doPostConnect(m_mockMessageSender, m_mockPostConnectObserver);

    promiseFuturePair.waitFor(SHORT_DELAY);
}

/**
 * Check if doPostConnect() gets called twice in a row, the method returns false..
 */
TEST_F(PostConnectSequencerTest, test_doPostConnectReturnFalseOnSecondCall) {
    auto operation1 = std::make_shared<StrictMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(1));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);

    auto postConnectSequencer = PostConnectSequencer::create(operationsSet);
    ASSERT_NE(postConnectSequencer, nullptr);

    EXPECT_CALL(*operation1, performOperation(_)).WillOnce(Return(true));

    PromiseFuturePair<bool> promiseFuturePair;
    EXPECT_CALL(*m_mockPostConnectObserver, onPostConnected()).WillOnce(Invoke([&promiseFuturePair] {
        promiseFuturePair.setValue(true);
    }));

    ASSERT_TRUE(postConnectSequencer->doPostConnect(m_mockMessageSender, m_mockPostConnectObserver));
    ASSERT_FALSE(postConnectSequencer->doPostConnect(m_mockMessageSender, m_mockPostConnectObserver));

    promiseFuturePair.waitFor(SHORT_DELAY);
}

/**
 * Check if PostConnectSequencer stops execution of PostConnectOperations if the performOperation() fails on one of
 * them.
 */
TEST_F(PostConnectSequencerTest, test_subsequentOperationsDontExecute) {
    auto operation1 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation2 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation3 = std::make_shared<StrictMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(3));
    EXPECT_CALL(*operation2, getOperationPriority()).WillRepeatedly(Return(2));
    EXPECT_CALL(*operation3, getOperationPriority()).WillRepeatedly(Return(1));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);
    operationsSet.insert(operation2);
    operationsSet.insert(operation3);

    auto postConnectSequencer = PostConnectSequencer::create(operationsSet);
    ASSERT_NE(postConnectSequencer, nullptr);

    EXPECT_CALL(*operation3, performOperation(_)).WillOnce(Return(false));

    PromiseFuturePair<bool> promiseFuturePair;
    EXPECT_CALL(*m_mockPostConnectObserver, onUnRecoverablePostConnectFailure()).WillOnce(Invoke([&promiseFuturePair] {
        promiseFuturePair.setValue(true);
    }));

    postConnectSequencer->doPostConnect(m_mockMessageSender, m_mockPostConnectObserver);

    /// It is possible that the destructor gets called first which initiates an abortOperation before the mainLoopThread
    /// exits.
    EXPECT_CALL(*operation3, abortOperation()).Times(AtMost(1));

    promiseFuturePair.waitFor(SHORT_DELAY);
}

/**
 * Check if onDisconnect() stops execution of PostConnectOperations.
 */
TEST_F(PostConnectSequencerTest, test_onDisconnectStopsExecution) {
    auto operation1 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation2 = std::make_shared<StrictMock<MockPostConnectOperation>>();
    auto operation3 = std::make_shared<StrictMock<MockPostConnectOperation>>();

    EXPECT_CALL(*operation1, getOperationPriority()).WillRepeatedly(Return(3));
    EXPECT_CALL(*operation2, getOperationPriority()).WillRepeatedly(Return(2));
    EXPECT_CALL(*operation3, getOperationPriority()).WillRepeatedly(Return(1));

    PostConnectSequencer::PostConnectOperationsSet operationsSet;
    operationsSet.insert(operation1);
    operationsSet.insert(operation2);
    operationsSet.insert(operation3);

    auto postConnectSequencer = PostConnectSequencer::create(operationsSet);
    ASSERT_NE(postConnectSequencer, nullptr);

    PromiseFuturePair<bool> notifyOnPerfromOperation, notifyOnAbortOperation;
    EXPECT_CALL(*operation3, performOperation(_))
        .WillOnce(Invoke([&notifyOnAbortOperation,
                          &notifyOnPerfromOperation](const std::shared_ptr<MessageSenderInterface>& postConnectSender) {
            notifyOnPerfromOperation.setValue(true);
            notifyOnAbortOperation.waitFor(SHORT_DELAY);
            return true;
        }));
    EXPECT_CALL(*operation3, abortOperation()).WillOnce(Invoke([&notifyOnAbortOperation] {
        notifyOnAbortOperation.setValue(true);
    }));

    postConnectSequencer->doPostConnect(m_mockMessageSender, m_mockPostConnectObserver);
    notifyOnPerfromOperation.waitFor(SHORT_DELAY);
    postConnectSequencer->onDisconnect();
}

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK
