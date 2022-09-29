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

#include <gtest/gtest.h>

#include <acsdk/Notifier/private/ObserverWrapper.h>

namespace alexaClientSDK {
namespace notifier {
namespace test {

/**
 * @brief Test fixture for @a ObserverWrapper unit tests.
 * @ingroup Lib_acsdkNotifierTests
 */
class ObserverWrapperTest : public ::testing::Test {};

/**
 * @brief Test class for unit tests.
 * @ingroup Lib_acsdkNotifierTests
 * @private
 */
class TestObserver {};

TEST_F(ObserverWrapperTest, test_strongRefState) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper(ReferenceType::StrongRef, strongRef);
    ASSERT_EQ(ReferenceType::StrongRef, wrapper.getReferenceType());
    ASSERT_EQ(strongRef, wrapper.get());
    ASSERT_TRUE(wrapper.isEqualOrExpired(strongRef.get()));
    ASSERT_FALSE(wrapper.isEqualOrExpired(std::make_shared<TestObserver>().get()));
}

TEST_F(ObserverWrapperTest, test_strongReferenceDoesNotExpire) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper(ReferenceType::StrongRef, strongRef);
    std::weak_ptr<TestObserver> weakPtr = strongRef;
    strongRef.reset();
    ASSERT_FALSE(weakPtr.expired());
    ASSERT_FALSE(wrapper.isEqualOrExpired(nullptr));
}

TEST_F(ObserverWrapperTest, test_weakReferenceExpires) {
    auto testObserver = std::make_shared<TestObserver>();
    ObserverWrapper wrapper(ReferenceType::WeakRef, testObserver);
    std::weak_ptr<TestObserver> weakRef = testObserver;
    ASSERT_FALSE(weakRef.expired());
    testObserver.reset();
    ASSERT_TRUE(weakRef.expired());
    ASSERT_TRUE(wrapper.isEqualOrExpired(std::make_shared<TestObserver>().get()));
    ASSERT_TRUE(wrapper.isEqualOrExpired(nullptr));
}

TEST_F(ObserverWrapperTest, test_copyConstructStrongRef) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper1(ReferenceType::StrongRef, strongRef);
    ObserverWrapper wrapper2(wrapper1);

    ASSERT_EQ(ReferenceType::StrongRef, wrapper1.getReferenceType());
    ASSERT_EQ(ReferenceType::StrongRef, wrapper2.getReferenceType());
    ASSERT_EQ(strongRef, wrapper1.get());
    ASSERT_EQ(strongRef, wrapper2.get());
}

TEST_F(ObserverWrapperTest, test_moveConstructStrongRef) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper1(ReferenceType::StrongRef, strongRef);
    ObserverWrapper wrapper2(std::move(wrapper1));

    ASSERT_EQ(ReferenceType::StrongRef, wrapper1.getReferenceType());
    ASSERT_EQ(ReferenceType::StrongRef, wrapper2.getReferenceType());
    ASSERT_EQ(nullptr, wrapper1.get());
    ASSERT_EQ(strongRef, wrapper2.get());
}

TEST_F(ObserverWrapperTest, test_copyConstructWeakRef) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper1(ReferenceType::WeakRef, strongRef);
    ObserverWrapper wrapper2(wrapper1);

    ASSERT_EQ(ReferenceType::WeakRef, wrapper1.getReferenceType());
    ASSERT_EQ(ReferenceType::WeakRef, wrapper2.getReferenceType());
    ASSERT_EQ(strongRef, wrapper1.get());
    ASSERT_EQ(strongRef, wrapper2.get());
}

TEST_F(ObserverWrapperTest, test_moveConstructWeakRef) {
    auto strongRef = std::make_shared<TestObserver>();
    ObserverWrapper wrapper1(ReferenceType::WeakRef, strongRef);
    ObserverWrapper wrapper2(std::move(wrapper1));

    ASSERT_EQ(ReferenceType::WeakRef, wrapper1.getReferenceType());
    ASSERT_EQ(ReferenceType::WeakRef, wrapper2.getReferenceType());
    ASSERT_EQ(nullptr, wrapper1.get().get());
    ASSERT_EQ(strongRef, wrapper2.get());
}

}  // namespace test
}  // namespace notifier
}  // namespace alexaClientSDK
