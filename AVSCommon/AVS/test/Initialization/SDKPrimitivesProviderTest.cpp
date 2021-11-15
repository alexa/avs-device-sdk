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

/// @file SDKPrimitivesProviderTest.cpp

#include <sstream>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/Initialization/SDKPrimitivesProvider.h"
#include "AVSCommon/Utils/Timing/TimerDelegateFactory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {
namespace test {

using namespace std;

/// Test harness for @c AlexaClientSDKInit class.
class SDKPrimitivesProviderTest : public ::testing::Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

    /// The SDKPrimitivesProvider instance to be tested.
    std::shared_ptr<SDKPrimitivesProvider> m_primitivesProvider;

    /// The SDKPrimitivesProviderCopy instance to be tested.
    std::shared_ptr<SDKPrimitivesProvider> m_primitivesProviderCopy;
};

void SDKPrimitivesProviderTest::SetUp() {
    m_primitivesProvider = SDKPrimitivesProvider::getInstance();
    m_primitivesProviderCopy = SDKPrimitivesProvider::getInstance();
    ASSERT_NE(m_primitivesProvider, nullptr);
    ASSERT_NE(m_primitivesProviderCopy, nullptr);
}

void SDKPrimitivesProviderTest::TearDown() {
    m_primitivesProvider->terminate();
    m_primitivesProviderCopy->terminate();
}

/**
 * Tests @c getInstance and verifies that it is not created initialized, expecting to return @c false.
 */
TEST_F(SDKPrimitivesProviderTest, test_getInstanceNotInitialized) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
}

/**
 * Tests @c getInstance and verifies that it is not created initialized using multiple references, expecting to return
 * @c false from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_getInstanceMultipleNotInitialized) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_FALSE(m_primitivesProviderCopy->isInitialized());
}

/**
 * Tests @c getInstance and verifies that only a singleton is created, expecting both pointers to point to same object.
 */
TEST_F(SDKPrimitivesProviderTest, test_getInstanceSingleton) {
    ASSERT_EQ(m_primitivesProvider, m_primitivesProviderCopy);
}

/**
 * Tests @c initialize and verifies that it does not initialize twice, expecting to return @c false.
 */
TEST_F(SDKPrimitivesProviderTest, test_initializeOnlyOnce) {
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_FALSE(m_primitivesProvider->initialize());
}

/**
 * Tests @c initialize and verifies that it does not initialize twice using multiple references, expecting to return @c
 * false from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_initializeOnlyOnceUsingMultipleReferences) {
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_FALSE(m_primitivesProvider->initialize());
    ASSERT_FALSE(m_primitivesProviderCopy->initialize());
}

/**
 * Tests @c withTimerDelegateFactory, expecting to return @c true.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactory) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_TRUE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
}

/**
 * Tests @c withTimerDelegateFactory using multiple references, expecting to return @c true from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactoryUsingMultipleReferences) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_TRUE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
    ASSERT_TRUE(m_primitivesProviderCopy->withTimerDelegateFactory(timerDelegateFactory));
}

/**
 * Tests @c withTimerDelegateFactory with null TimerDelegateFactory, expecting to return @c false.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactoryNull) {
    ASSERT_FALSE(m_primitivesProvider->withTimerDelegateFactory(nullptr));
}

/**
 * Tests @c withTimerDelegateFactory with null TimerDelegateFactory using multiple references, expecting to return @c
 * false from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactoryNullUsingMultipleReferences) {
    ASSERT_FALSE(m_primitivesProvider->withTimerDelegateFactory(nullptr));
    ASSERT_FALSE(m_primitivesProviderCopy->withTimerDelegateFactory(nullptr));
}

/**
 * Tests @c withTimerDelegateFactory already initialized, expecting to return @c false.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactoryInitialized) {
    ASSERT_TRUE(m_primitivesProvider->initialize());
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_FALSE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
}

/**
 * Tests @c withTimerDelegateFactory already initialized using multiple references, expecting to return @c false from
 * all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_withTimerDelegateFactoryInitializedMultipleReferences) {
    ASSERT_TRUE(m_primitivesProvider->initialize());
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_FALSE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
    ASSERT_FALSE(m_primitivesProviderCopy->withTimerDelegateFactory(timerDelegateFactory));
}

/**
 * Tests @c getTimerDelegateFactory, expecting no errors.
 */
TEST_F(SDKPrimitivesProviderTest, test_getTimerDelegateFactory) {
    m_primitivesProvider->initialize();
    m_primitivesProvider->getTimerDelegateFactory();
}

/**
 * Tests @c getTimerDelegateFactory, expecting to return the same object.
 */
TEST_F(SDKPrimitivesProviderTest, test_getTimerDelegateFactoryManual) {
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_TRUE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
    ASSERT_NE(m_primitivesProvider->getTimerDelegateFactory(), timerDelegateFactory);
    m_primitivesProvider->initialize();
    ASSERT_EQ(m_primitivesProvider->getTimerDelegateFactory(), timerDelegateFactory);
}

/**
 * Tests @c getTimerDelegateFactory using multiple references, expecting to return the same object from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_getTimerDelegateFactoryManualMultipleReferences) {
    auto timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
    ASSERT_TRUE(m_primitivesProvider->withTimerDelegateFactory(timerDelegateFactory));
    ASSERT_NE(m_primitivesProvider->getTimerDelegateFactory(), timerDelegateFactory);
    m_primitivesProvider->initialize();
    ASSERT_EQ(m_primitivesProvider->getTimerDelegateFactory(), timerDelegateFactory);
    ASSERT_EQ(m_primitivesProviderCopy->getTimerDelegateFactory(), timerDelegateFactory);
}

/**
 * Tests @c getTimerDelegateFactory without initialization, expecting to return @c nullptr.
 */
TEST_F(SDKPrimitivesProviderTest, test_getTimerDelegateFactoryWithoutInitialzation) {
    ASSERT_EQ(m_primitivesProvider->getTimerDelegateFactory(), nullptr);
}

/**
 * Tests @c getTimerDelegateFactory without initialization using multiple references, expecting to return @c nullptr
 * from all references.
 */
TEST_F(SDKPrimitivesProviderTest, test_getTimerDelegateFactoryWithoutInitialzationMultipleReferences) {
    ASSERT_EQ(m_primitivesProvider->getTimerDelegateFactory(), nullptr);
    ASSERT_EQ(m_primitivesProviderCopy->getTimerDelegateFactory(), nullptr);
}

/**
 * Tests @c isInitialized and verifies that it behaves correctly after initialization.
 */
TEST_F(SDKPrimitivesProviderTest, test_isInitialized) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_TRUE(m_primitivesProvider->isInitialized());
    m_primitivesProvider->terminate();
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
}

/**
 * Tests @c reset and verify that it correctly resets the timerDelegateFactory.
 */
TEST_F(SDKPrimitivesProviderTest, test_resetTimerDelegateFactory) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_TRUE(m_primitivesProvider->isInitialized());
    m_primitivesProvider->reset();
    ASSERT_EQ(m_primitivesProvider->getTimerDelegateFactory(), nullptr);
}

/**
 * Tests @c reset and verify that it correctly uninititalizes.
 */
TEST_F(SDKPrimitivesProviderTest, test_resetUninitializes) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_TRUE(m_primitivesProvider->isInitialized());
    m_primitivesProvider->reset();
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
}

/**
 * Tests @c reset using multiple references and verify that it correctly uninititalizes.
 */
TEST_F(SDKPrimitivesProviderTest, test_resetUninitializesMultipleReferences) {
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_TRUE(m_primitivesProvider->initialize());
    ASSERT_TRUE(m_primitivesProvider->isInitialized());
    m_primitivesProvider->reset();
    ASSERT_FALSE(m_primitivesProvider->isInitialized());
    ASSERT_FALSE(m_primitivesProviderCopy->isInitialized());
}

/**
 * Tests @c terminate without initialization and makes sure that multiple calls do not cause errors.
 */
TEST_F(SDKPrimitivesProviderTest, test_terminateMultipleTimesWithoutInitialization) {
    ASSERT_EQ(m_primitivesProvider, m_primitivesProviderCopy);
    m_primitivesProvider->terminate();
    m_primitivesProviderCopy->terminate();
}

/**
 * Tests @c terminate with initialization and makes sure that multiple calls do not cause errors.
 */
TEST_F(SDKPrimitivesProviderTest, test_terminateMultipleTimesWithInitialization) {
    m_primitivesProvider->initialize();
    ASSERT_EQ(m_primitivesProvider, m_primitivesProviderCopy);
    m_primitivesProvider->terminate();
    m_primitivesProviderCopy->terminate();
}

}  // namespace test
}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK