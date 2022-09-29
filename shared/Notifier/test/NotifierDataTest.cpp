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

#include <acsdk/Notifier/private/NotifierData.h>

namespace alexaClientSDK {
namespace notifier {
namespace test {

using namespace ::testing;

/**
 * @brief Test fixture for @c NotifierData.
 * @ingroup Lib_acsdkNotifierTests
 */
class NotifierDataTest : public ::testing::Test {};

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
 *
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
class MockTestObserver : public TestObserverInterface {
public:
    MOCK_METHOD0(onSomething, void());
};

/// Alias for traits helper.
/// @ingroup Lib_acsdkNotifierTests
/// @private
using Traits = NotifierTraits<TestObserverInterface>;

/**
 * @brief Lambda to invoke observer method.
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
static auto invokeOnSomething = [](const std::shared_ptr<TestObserverInterface>& observer) { observer->onSomething(); };

/**
 * Verify factory returns valid pointer.
 */
TEST_F(NotifierDataTest, test_factory) {
    auto data = createDataInterface();
    ASSERT_TRUE(data);
}

TEST_F(NotifierDataTest, test_addStrongReferenceAndRemove) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(0);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_addStrongReferenceTwiceAndRemove) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(0);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_addWeakReferenceAndRemove) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(0);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_addWeakReferenceTwiceAndRemove) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(0);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_addStrongAndWeakReferenceAndRemove) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(0);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_addWeakReferenceAndExpire) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    std::weak_ptr<TestObserverInterface> weakRef = observer;
    EXPECT_FALSE(weakRef.expired());
    observer.reset();
    ASSERT_TRUE(weakRef.expired());
}

TEST_F(NotifierDataTest, test_addWeakAndStrongReferenceAndExpire) {
    auto data = createDataInterface();
    auto observer = std::make_shared<MockTestObserver>();
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer));
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    std::weak_ptr<TestObserverInterface> weakRef = observer;
    EXPECT_FALSE(weakRef.expired());
    observer.reset();
    ASSERT_TRUE(weakRef.expired());
}

TEST_F(NotifierDataTest, test_notifyInFifoOrderWithRemoval) {
    auto data = createDataInterface();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto observer3 = std::make_shared<MockTestObserver>();
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer1));
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer2));
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer3));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer1, onSomething()).Times(1);
        EXPECT_CALL(*observer2, onSomething()).Times(1);
        EXPECT_CALL(*observer3, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer2));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer1, onSomething()).Times(1);
        EXPECT_CALL(*observer2, onSomething()).Times(0);
        EXPECT_CALL(*observer3, onSomething()).Times(1);
        data->doNotifyObservers(Traits::adaptFunction(invokeOnSomething));
    }
}

TEST_F(NotifierDataTest, test_notifyInLifoOrderWithRemoval) {
    auto data = createDataInterface();
    auto observer1 = std::make_shared<MockTestObserver>();
    auto observer2 = std::make_shared<MockTestObserver>();
    auto observer3 = std::make_shared<MockTestObserver>();
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer1));
    data->doAddWeakRefObserver(Traits::toVoidPtr(observer2));
    data->doAddStrongRefObserver(Traits::toVoidPtr(observer3));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer3, onSomething()).Times(1);
        EXPECT_CALL(*observer2, onSomething()).Times(1);
        EXPECT_CALL(*observer1, onSomething()).Times(1);
        data->doNotifyObserversInReverse(Traits::adaptFunction(invokeOnSomething));
    }
    data->doRemoveObserver(Traits::toVoidPtr(observer2));
    {
        InSequence inSequence;
        EXPECT_CALL(*observer3, onSomething()).Times(1);
        EXPECT_CALL(*observer2, onSomething()).Times(0);
        EXPECT_CALL(*observer1, onSomething()).Times(1);
        data->doNotifyObserversInReverse(Traits::adaptFunction(invokeOnSomething));
    }
}

}  // namespace test
}  // namespace notifier
}  // namespace alexaClientSDK
