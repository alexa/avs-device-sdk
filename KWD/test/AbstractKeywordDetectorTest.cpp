/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <gmock/gmock.h>

#include <unordered_set>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

#include "KWD/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {
namespace test {

using ::testing::_;

/// A test observer that mocks out the KeyWordObserverInterface##onKeyWordDetected() call.
class MockKeyWordObserver : public avsCommon::sdkInterfaces::KeyWordObserverInterface {
public:
    MOCK_METHOD5(
        onKeyWordDetected,
        void(
            std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
            std::string keyword,
            avsCommon::avs::AudioInputStream::Index beginIndex,
            avsCommon::avs::AudioInputStream::Index endIndex,
            std::shared_ptr<const std::vector<char>> KWDMetadata));
};

/// A test observer that mocks out the KeyWordDetectorStateObserverInterface##onStateChanged() call.
class MockStateObserver : public avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface {
public:
    MOCK_METHOD1(
        onStateChanged,
        void(avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState
                 keyWordDetectorState));
};

/**
 * A mock Keyword Detector that inherits from KeyWordDetector.
 */
class MockKeyWordDetector : public AbstractKeywordDetector {
public:
    /**
     * Notifies all KeyWordObservers with dummy values.
     */
    void sendKeyWordCallToObservers() {
        notifyKeyWordObservers(nullptr, "ALEXA", 0, 0);
    };

    /**
     * Notifies all KeyWordDetectorStateObservers.
     *
     * @param state The state to notify observers of.
     */
    void sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState state) {
        notifyKeyWordDetectorStateObservers(state);
    };
};

class AbstractKeyWordDetectorTest : public ::testing::Test {
protected:
    std::shared_ptr<MockKeyWordDetector> detector;
    std::shared_ptr<MockKeyWordObserver> keyWordObserver1;
    std::shared_ptr<MockKeyWordObserver> keyWordObserver2;
    std::shared_ptr<MockStateObserver> stateObserver1;
    std::shared_ptr<MockStateObserver> stateObserver2;

    virtual void SetUp() {
        detector = std::make_shared<MockKeyWordDetector>();
        keyWordObserver1 = std::make_shared<MockKeyWordObserver>();
        keyWordObserver2 = std::make_shared<MockKeyWordObserver>();
        stateObserver1 = std::make_shared<MockStateObserver>();
        stateObserver2 = std::make_shared<MockStateObserver>();
    }
};

TEST_F(AbstractKeyWordDetectorTest, testAddKeyWordObserver) {
    detector->addKeyWordObserver(keyWordObserver1);

    EXPECT_CALL(*keyWordObserver1, onKeyWordDetected(_, _, _, _, _)).Times(1);
    detector->sendKeyWordCallToObservers();
}

TEST_F(AbstractKeyWordDetectorTest, testAddMultipleKeyWordObserver) {
    detector->addKeyWordObserver(keyWordObserver1);
    detector->addKeyWordObserver(keyWordObserver2);

    EXPECT_CALL(*keyWordObserver1, onKeyWordDetected(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*keyWordObserver2, onKeyWordDetected(_, _, _, _, _)).Times(1);
    detector->sendKeyWordCallToObservers();
}

TEST_F(AbstractKeyWordDetectorTest, testRemoveKeyWordObserver) {
    detector->addKeyWordObserver(keyWordObserver1);
    detector->addKeyWordObserver(keyWordObserver2);

    EXPECT_CALL(*keyWordObserver1, onKeyWordDetected(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*keyWordObserver2, onKeyWordDetected(_, _, _, _, _)).Times(1);
    detector->sendKeyWordCallToObservers();

    detector->removeKeyWordObserver(keyWordObserver1);

    EXPECT_CALL(*keyWordObserver1, onKeyWordDetected(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*keyWordObserver2, onKeyWordDetected(_, _, _, _, _)).Times(1);
    detector->sendKeyWordCallToObservers();
}

TEST_F(AbstractKeyWordDetectorTest, testAddStateObserver) {
    detector->addKeyWordDetectorStateObserver(stateObserver1);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(1);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
}

TEST_F(AbstractKeyWordDetectorTest, testAddMultipleStateObservers) {
    detector->addKeyWordDetectorStateObserver(stateObserver1);
    detector->addKeyWordDetectorStateObserver(stateObserver2);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(1);
    EXPECT_CALL(*stateObserver2, onStateChanged(_)).Times(1);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
}

TEST_F(AbstractKeyWordDetectorTest, testRemoveStateObserver) {
    detector->addKeyWordDetectorStateObserver(stateObserver1);
    detector->addKeyWordDetectorStateObserver(stateObserver2);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(1);
    EXPECT_CALL(*stateObserver2, onStateChanged(_)).Times(1);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

    detector->removeKeyWordDetectorStateObserver(stateObserver1);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(0);
    EXPECT_CALL(*stateObserver2, onStateChanged(_)).Times(1);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
}

TEST_F(AbstractKeyWordDetectorTest, testObserversDontGetNotifiedOfSameStateTwice) {
    detector->addKeyWordDetectorStateObserver(stateObserver1);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(1);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

    EXPECT_CALL(*stateObserver1, onStateChanged(_)).Times(0);
    detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
}

}  // namespace test
}  // namespace kwd
}  // namespace alexaClientSDK
