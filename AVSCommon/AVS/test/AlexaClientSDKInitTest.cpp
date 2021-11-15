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

/// @file AlexaClientSDKInitTest.cpp

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/Timing/MockTimerDelegateFactory.h>
#ifdef ENABLE_LPM
#include <AVSCommon/Utils/Power/NoOpPowerResourceManager.h>
#endif

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {
namespace test {

static std::vector<std::shared_ptr<std::istream>> EMPTY_JSON_STREAMS;

using namespace sdkInterfaces::test;
using namespace ::testing;

/// Test harness for @c AlexaClientSDKInit class.
class AlexaClientSDKInitTest : public ::testing::Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

    /// Test initialization parameters builder.
    std::unique_ptr<InitializationParametersBuilder> m_builder;
    /// Test json streams.
    std::shared_ptr<std::vector<std::shared_ptr<std::istream>>> m_jsonStreamsPtr;
    /// Test logger.
    std::shared_ptr<utils::logger::Logger> m_logger;
};

void AlexaClientSDKInitTest::SetUp() {
    m_builder = InitializationParametersBuilder::create();
    m_jsonStreamsPtr = std::make_shared<std::vector<std::shared_ptr<std::istream>>>(EMPTY_JSON_STREAMS);
    m_builder->withJsonStreams(m_jsonStreamsPtr);
}

void AlexaClientSDKInitTest::TearDown() {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Tests @c initialize without any initialization parameters, expecting to return @c false.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeNoInitParams) {
    ASSERT_FALSE(AlexaClientSDKInit::initialize(nullptr));
}

/**
 * Tests @c initialize with a null timerDelegateFactory, expecting to return @c false.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeWithNullTimerDelegateFactory) {
    auto initParams = m_builder->build();
    initParams->timerDelegateFactory = nullptr;
    ASSERT_FALSE(AlexaClientSDKInit::initialize(std::move(initParams)));
}

/**
 * Tests @c initialize without any JSON configuration, expecting to return @c true.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeNoJSONConfig) {
    ASSERT_TRUE(AlexaClientSDKInit::initialize(EMPTY_JSON_STREAMS));
}

#ifdef ENABLE_LPM
/*
 * Tests @c initialize with null Low Power Mode, expecting to return @c true.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeNullLPM) {
    auto initParams = m_builder->build();
    initParams->powerResourceManager = nullptr;
    ASSERT_TRUE(AlexaClientSDKInit::initialize(std::move(initParams)));
}

/*
 * Tests @c initialize with Low Power Mode enabled and unsupported TimerDelegateFactory, expecting to return @c false.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeLPMUnsupportedTimerDelegateFactory) {
    auto powerResourceManager = std::make_shared<avsCommon::utils::power::NoOpPowerResourceManager>();
    auto mockTimerDelegateFactory = std::make_shared<MockTimerDelegateFactory>();
    m_builder->withTimerDelegateFactory(mockTimerDelegateFactory);
    EXPECT_CALL(*mockTimerDelegateFactory, supportsLowPowerMode()).WillOnce(Return(false));
    m_builder->withPowerResourceManager(powerResourceManager);
    auto initParams = m_builder->build();
    ASSERT_FALSE(AlexaClientSDKInit::initialize(std::move(initParams)));
}
#endif

/**
 * Tests @c initialize with an invalid JSON configuration, expecting to return @c false.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeInvalidJSONConfig) {
    auto invalidJSON = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*invalidJSON) << "{";
    ASSERT_FALSE(AlexaClientSDKInit::initialize({invalidJSON}));
}

/**
 * Tests @c initialize with a valid JSON configuration, expecting to return @c true.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST_F(AlexaClientSDKInitTest, test_initializeValidJSONConfig) {
    auto validJSON = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*validJSON) << R"({"key":"value"})";
    ASSERT_TRUE(AlexaClientSDKInit::initialize({validJSON}));
}

/**
 * Tests @c isInitialized when the SDK has not been initialized yet, expecting to return @c false.
 */
TEST_F(AlexaClientSDKInitTest, test_uninitializedIsInitialized) {
    ASSERT_FALSE(AlexaClientSDKInit::isInitialized());
}

/**
 * Tests @c isInitialized when the SDK is initialized, expecting to return @c true.
 */
TEST_F(AlexaClientSDKInitTest, test_isInitialized) {
    ASSERT_TRUE(AlexaClientSDKInit::initialize(EMPTY_JSON_STREAMS));
    EXPECT_TRUE(AlexaClientSDKInit::isInitialized());
}

/**
 * Tests @c uninitialize when the SDK has not been initialized yet, expecting no crashes or exceptions.
 */
TEST_F(AlexaClientSDKInitTest, test_uninitialize) {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Tests @c getCreateAlexaClientSDKInit using JSON Stream with a null logger, expecting to return @c nullptr.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitNullLoggerUsingJSON) {
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit(EMPTY_JSON_STREAMS);
    ASSERT_EQ(constructor(nullptr), nullptr);
}

/**
 * Tests @c getCreateAlexaClientSDKInit using Init Params with a null logger, expecting to return @c nullptr.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitNullLoggerUsingInitParams) {
    auto initParams = m_builder->build();
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams));
    ASSERT_EQ(constructor(nullptr), nullptr);
}

/**
 * Tests @c getCreateAlexaClientSDKInit using invalid JSON Stream, expecting to return @c nullptr.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitInvalidJSONStream) {
    auto invalidJSON = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*invalidJSON) << "{";
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit({invalidJSON});
    ASSERT_EQ(constructor(m_logger), nullptr);
}

/**
 * Tests @c getCreateAlexaClientSDKInit using valid JSON Stream, expecting to return @c true.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitValidJSONStream) {
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit(EMPTY_JSON_STREAMS);
    auto alexaClientSDKInitInstance = constructor(m_logger);
    ASSERT_FALSE(AlexaClientSDKInit::isInitialized());
    alexaClientSDKInitInstance->initialize(EMPTY_JSON_STREAMS);
    ASSERT_TRUE(AlexaClientSDKInit::isInitialized());
}

/**
 * Tests @c getCreateAlexaClientSDKInit using valid Init Params, expecting to return @c true.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitValidInitParams) {
    auto initParams = m_builder->build();
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams));
    auto alexaClientSDKInitInstance = constructor(m_logger);
    ASSERT_FALSE(AlexaClientSDKInit::isInitialized());
    alexaClientSDKInitInstance->initialize(EMPTY_JSON_STREAMS);
    ASSERT_TRUE(AlexaClientSDKInit::isInitialized());
}

/**
 * Tests @c getCreateAlexaClientSDKInit using null Init Params, expecting to return @c nullptr.
 */
TEST_F(AlexaClientSDKInitTest, test_getCreateAlexaClientSDKInitNullInitParams) {
    auto initParams = m_builder->build();
    auto constructor = AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams));
    ASSERT_EQ(constructor(m_logger), nullptr);
}

}  // namespace test
}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
