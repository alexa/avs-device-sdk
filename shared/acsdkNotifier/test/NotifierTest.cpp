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

/// @file NotifierTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkNotifier/Notifier.h"

namespace alexaClientSDK {
namespace acsdkNotifier {
namespace test {

using namespace ::testing;
using namespace acsdkNotifier;

class NotifierTest : public ::testing::Test {};

class TestObserverInterface {
public:
    virtual ~TestObserverInterface() = default;

    virtual void onSomething() = 0;
};

class MockTestObserver : public TestObserverInterface {
public:
    MOCK_METHOD0(onSomething, void());
};

using TestNotifier = Notifier<TestObserverInterface>;

static auto invokeOnSomething = [](const std::shared_ptr<TestObserverInterface>& observer) { observer->onSomething(); };

/**
 * Verify the simplest case of notifying an observer.
 */
TEST_F(NotifierTest, test_simplestNotification) {
    TestNotifier notifier;
    auto observer = std::make_shared<MockTestObserver>();
    EXPECT_CALL(*observer, onSomething());
    notifier.addObserver(observer);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify the order in which observers are notified.
 */
TEST_F(NotifierTest, test_notificationOrder) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    InSequence sequence;
    EXPECT_CALL(*observer0, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    EXPECT_CALL(*observer2, onSomething());
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify duplicate additions are ignored.
 */
TEST_F(NotifierTest, test_duplicateAdditions) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    InSequence sequence;
    EXPECT_CALL(*observer0, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    EXPECT_CALL(*observer2, onSomething());
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.addObserver(observer1);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify addObserverFunc is called on adding an observer when it is set.
 */
TEST_F(NotifierTest, test_setAddObserverFunction) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();

    bool result = false;
    std::function<void(const std::shared_ptr<TestObserverInterface>&)> addObserverFunction =
        [&result](const std::shared_ptr<TestObserverInterface>&) { result = true; };

    notifier.addObserver(observer0);
    ASSERT_EQ(result, false);

    notifier.setAddObserverFunction(addObserverFunction);

    auto observer1 = std::make_shared<MockTestObserver>();
    notifier.addObserver(observer1);
    ASSERT_EQ(result, true);
}

/**
 * Verify removal of observers.
 */
TEST_F(NotifierTest, test_removingObservers) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    InSequence sequence;
    EXPECT_CALL(*observer2, onSomething());
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.removeObserver(observer0);
    notifier.removeObserver(observer1);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify notification of observers in the reverse order.
 */
TEST_F(NotifierTest, test_notificationInReverseOrder) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    InSequence sequence;
    EXPECT_CALL(*observer2, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    EXPECT_CALL(*observer0, onSomething());
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.notifyObserversInReverse(invokeOnSomething);
}

/**
 * Verify removal of observer from within callback.
 */
TEST_F(NotifierTest, test_removeWithinCallback) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto removeObservers = [&observer0, &observer2, &notifier]() {
        notifier.removeObserver(observer0);
        notifier.removeObserver(observer2);
    };
    InSequence sequence;
    EXPECT_CALL(*observer0, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(removeObservers));
    EXPECT_CALL(*observer2, onSomething()).Times(0);
    EXPECT_CALL(*observer1, onSomething()).Times(1);
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.notifyObservers(invokeOnSomething);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify removal and addition of observers from within callback during reverse order notify.
 * Verify that removing an item not yet visited will result in the removed item not getting notified.
 * Verify that adding an item during notification will not result in the new item getting visited
 * (and that @c notifyObserversInReverse() will return false).
 */
TEST_F(NotifierTest, test_removeAndAdditionWithinReverseOrderCallback) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto removeObservers = [&observer0, &observer2, &notifier]() {
        notifier.removeObserver(observer0);
        notifier.removeObserver(observer2);
    };
    auto addObservers = [&observer0, &observer2, &notifier]() {
        notifier.addObserver(observer0);
        notifier.addObserver(observer2);
    };
    InSequence sequence;
    EXPECT_CALL(*observer2, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(removeObservers));
    EXPECT_CALL(*observer0, onSomething()).Times(0);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(addObservers));
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    ASSERT_TRUE(notifier.notifyObserversInReverse(invokeOnSomething));
    ASSERT_FALSE(notifier.notifyObserversInReverse(invokeOnSomething));
}

}  // namespace test
}  // namespace acsdkNotifier
}  // namespace alexaClientSDK
