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

#include "acsdk/Notifier/Notifier.h"

namespace alexaClientSDK {
namespace notifier {
namespace test {

using namespace ::testing;
using namespace notifier;

/**
 * @brief Test fixture for @c Notifier.
 * @ingroup Lib_acsdkNotifierTests
 */
class NotifierTest : public ::testing::Test {};

/**
 * @brief Test interface.
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
class TestObserverInterface {
public:
    virtual ~TestObserverInterface() = default;

    virtual void onSomething() = 0;
};

/**
 * @brief Mock for @c TestObserverInterface.
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
class MockTestObserver : public TestObserverInterface {
public:
    MOCK_METHOD0(onSomething, void());
};

using TestNotifier = Notifier<TestObserverInterface>;

/**
 * @brief Helper lambda to invoke TestObserverInterface::onSomething() method.
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
static const auto invokeOnSomething = [](const std::shared_ptr<TestObserverInterface>& observer) {
    observer->onSomething();
};

/**
 * Verify the simplest case of notifying an observer.
 */
TEST_F(NotifierTest, test_simplestNotification) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    EXPECT_CALL(*observer0, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
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
    auto observer3 = std::make_shared<MockTestObserver>();
    auto observer4 = std::make_shared<MockTestObserver>();
    auto observer5 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver3 = observer3;
    std::weak_ptr<MockTestObserver> weakObserver5 = observer5;

    InSequence sequence;
    EXPECT_CALL(*observer0, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    EXPECT_CALL(*observer2, onSomething());
    EXPECT_CALL(*observer3, onSomething());
    EXPECT_CALL(*observer4, onSomething());
    EXPECT_CALL(*observer5, onSomething());
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.addObserver(observer2);
    notifier.addWeakPtrObserver(weakObserver3);
    notifier.addObserver(observer4);
    notifier.addWeakPtrObserver(weakObserver5);
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
    std::weak_ptr<MockTestObserver> weakObserver0 = observer0;
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver2 = observer2;
    EXPECT_CALL(*observer0, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).Times(1);
    EXPECT_CALL(*observer2, onSomething()).Times(1);
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);
    notifier.addObserver(observer1);
    notifier.addWeakPtrObserver(weakObserver2);
    notifier.addWeakPtrObserver(weakObserver2);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify addObserverFunc is called on adding an observer when it is set before and after setAddObserverFunction.
 */
TEST_F(NotifierTest, test_setAddObserverFunction) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto observer3 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver3 = observer3;
    EXPECT_CALL(*observer0, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).Times(1);
    EXPECT_CALL(*observer2, onSomething()).Times(1);
    EXPECT_CALL(*observer3, onSomething()).Times(1);

    std::function<void(const std::shared_ptr<TestObserverInterface>&)> addObserverFunction =
        [](const std::shared_ptr<TestObserverInterface>& observer) { observer->onSomething(); };

    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.setAddObserverFunction(addObserverFunction);
    notifier.addObserver(observer2);
    notifier.addWeakPtrObserver(weakObserver3);
}

/**
 * Verify removal of observers.
 */
TEST_F(NotifierTest, test_removingObservers) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto observer3 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver3 = observer3;

    InSequence sequence;
    EXPECT_CALL(*observer2, onSomething());
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.addObserver(observer2);
    notifier.addWeakPtrObserver(weakObserver3);
    notifier.removeObserver(observer0);
    notifier.removeObserver(observer1);
    notifier.removeWeakPtrObserver(weakObserver3);
    notifier.notifyObservers(invokeOnSomething);
}

/**
 * Verify removal of observers.
 */
TEST_F(NotifierTest, test_removingObserversWithNotificationInFifoMode) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();

    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);

    {
        InSequence sequence;
        EXPECT_CALL(*observer0, onSomething());
        EXPECT_CALL(*observer1, onSomething());
        EXPECT_CALL(*observer2, onSomething());

        notifier.notifyObservers(invokeOnSomething);
    }
    notifier.removeObserver(observer1);
    {
        InSequence sequence;
        EXPECT_CALL(*observer0, onSomething());
        EXPECT_CALL(*observer1, onSomething()).Times(0);
        EXPECT_CALL(*observer2, onSomething());

        notifier.notifyObservers(invokeOnSomething);
    }
}

/**
 * Verify removal of observers.
 */
TEST_F(NotifierTest, test_removingObserversWithNotificationInLifoMode) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();

    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addObserver(observer2);

    {
        InSequence sequence;
        EXPECT_CALL(*observer2, onSomething());
        EXPECT_CALL(*observer1, onSomething());
        EXPECT_CALL(*observer0, onSomething());

        notifier.notifyObserversInReverse(invokeOnSomething);
    }
    notifier.removeObserver(observer1);
    {
        InSequence sequence;
        EXPECT_CALL(*observer2, onSomething());
        EXPECT_CALL(*observer1, onSomething()).Times(0);
        EXPECT_CALL(*observer0, onSomething());

        notifier.notifyObserversInReverse(invokeOnSomething);
    }
}

/**
 * Verify notification of observers in the reverse order.
 */
TEST_F(NotifierTest, test_notificationInReverseOrder) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto observer3 = std::make_shared<MockTestObserver>();
    auto observer4 = std::make_shared<MockTestObserver>();
    auto observer5 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver3 = observer3;
    std::weak_ptr<MockTestObserver> weakObserver5 = observer5;

    InSequence sequence;
    EXPECT_CALL(*observer5, onSomething());
    EXPECT_CALL(*observer4, onSomething());
    EXPECT_CALL(*observer3, onSomething());
    EXPECT_CALL(*observer2, onSomething());
    EXPECT_CALL(*observer1, onSomething());
    EXPECT_CALL(*observer0, onSomething());
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.addObserver(observer2);
    notifier.addWeakPtrObserver(weakObserver3);
    notifier.addObserver(observer4);
    notifier.addWeakPtrObserver(weakObserver5);
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
    auto observer3 = std::make_shared<MockTestObserver>();
    auto observer4 = std::make_shared<MockTestObserver>();
    auto observer5 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    std::weak_ptr<MockTestObserver> weakObserver3 = observer3;
    std::weak_ptr<MockTestObserver> weakObserver5 = observer5;
    auto removeObservers = [&observer0, &observer2, &weakObserver3, &notifier]() {
        notifier.removeObserver(observer0);
        notifier.removeObserver(observer2);
        notifier.removeWeakPtrObserver(weakObserver3);
    };
    InSequence sequence;
    notifier.addObserver(observer0);
    notifier.addWeakPtrObserver(weakObserver1);
    notifier.addObserver(observer2);
    notifier.addWeakPtrObserver(weakObserver3);
    notifier.addObserver(observer4);
    notifier.addWeakPtrObserver(weakObserver5);

    EXPECT_CALL(*observer0, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(removeObservers));
    EXPECT_CALL(*observer2, onSomething()).Times(0);
    EXPECT_CALL(*observer3, onSomething()).Times(0);
    EXPECT_CALL(*observer4, onSomething()).Times(1);
    EXPECT_CALL(*observer5, onSomething()).Times(1);
    notifier.notifyObservers(invokeOnSomething);

    EXPECT_CALL(*observer1, onSomething()).Times(1);
    EXPECT_CALL(*observer4, onSomething()).Times(1);
    EXPECT_CALL(*observer5, onSomething()).Times(1);
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
    std::weak_ptr<MockTestObserver> weakObserver2 = observer2;
    auto removeObservers = [&observer0, &weakObserver2, &notifier]() {
        notifier.removeObserver(observer0);
        notifier.removeWeakPtrObserver(weakObserver2);
    };
    auto addObservers = [&observer0, &weakObserver2, &notifier]() {
        notifier.addObserver(observer0);
        notifier.addWeakPtrObserver(weakObserver2);
    };
    InSequence sequence;
    EXPECT_CALL(*observer2, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(removeObservers));
    EXPECT_CALL(*observer0, onSomething()).Times(0);
    EXPECT_CALL(*observer1, onSomething()).WillOnce(Invoke(addObservers));
    notifier.addObserver(observer0);
    notifier.addObserver(observer1);
    notifier.addWeakPtrObserver(weakObserver2);
    ASSERT_TRUE(notifier.notifyObserversInReverse(invokeOnSomething));
    ASSERT_FALSE(notifier.notifyObserversInReverse(invokeOnSomething));
}

/**
 * Verify that when weak_ptr observer is expired (the underlying shared_ptr is reset), that the weak_ptr observer will
 * not get the notification.
 */
TEST_F(NotifierTest, test_resetSharedPtrWeakPtrCallbackShallNotBeCalled) {
    TestNotifier notifier;
    auto observer0 = std::make_shared<MockTestObserver>();
    auto observer1 = std::make_shared<MockTestObserver>();
    std::weak_ptr<MockTestObserver> weakObserver0 = observer0;
    std::weak_ptr<MockTestObserver> weakObserver1 = observer1;
    int count = 0;

    auto invokeCallback = [&count](const std::shared_ptr<TestObserverInterface>& observer) {
        count++;
        observer->onSomething();
    };

    InSequence sequence;
    EXPECT_CALL(*observer0, onSomething()).Times(1);
    EXPECT_CALL(*observer1, onSomething()).Times(2);
    notifier.addWeakPtrObserver(observer0);
    notifier.addWeakPtrObserver(observer1);
    notifier.notifyObservers(invokeCallback);
    observer0.reset();
    notifier.notifyObservers(invokeCallback);
    ASSERT_EQ(3, count);
}

}  // namespace test
}  // namespace notifier
}  // namespace alexaClientSDK
