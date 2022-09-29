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
#include <gmock/gmock.h>

#include <memory>

#include <acsdkKWDInterfaces/KeywordDetectorStateNotifierInterface.h>
#include <acsdkKWDInterfaces/KeywordNotifierInterface.h>
#include <acsdk/NotifierInterfaces/test/MockNotifier.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>

#include "acsdkKWDImplementations/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace acsdkKWDImplementations {
namespace test {

/// Reader timeout.
static constexpr std::chrono::milliseconds TIMEOUT{1000};

/// The size of reader buffer is one page long.
static constexpr size_t TEST_BUFFER_SIZE{4096u};

// No Words read from buffer.
static constexpr ssize_t ZERO_WORDS_READ = 0;

// Number of words to read from buffer.
static constexpr ssize_t WORDS_TO_READ = 1;

using namespace ::testing;

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

/// A test KeywordNotifier.
class MockKeywordNotifier
        : public notifierInterfaces::test::MockNotifier<avsCommon::sdkInterfaces::KeyWordObserverInterface> {};

/// A test KeywordDetectorStateNotifier.
class MockKeywordDetectorStateNotifier
        : public notifierInterfaces::test::MockNotifier<
              avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface> {};

/**
 * A mock Keyword Detector that inherits from KeyWordDetector.
 */
class MockKeyWordDetector : public AbstractKeywordDetector {
public:
    /**
     * Constructor.
     *
     * @param keyWordNotifier The object with which to notifiy observers of keyword detections.
     * @param KeyWordDetectorStateNotifier The object with which to notify observers of state changes in the engine.
     */
    MockKeyWordDetector(
        std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
        std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> keywordDetectorStateNotifier) :
            AbstractKeywordDetector(keywordNotifier, keywordDetectorStateNotifier) {
    }
    /**
     * Notifies all KeyWordObservers with dummy values.
     */
    void sendKeyWordCallToObservers();

    /**
     * Notifies all KeyWordDetectorStateObservers.
     *
     * @param state The state to notify observers of.
     */
    void sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState state);

    ssize_t protectedReadFromStream(
        std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        void* buf,
        size_t nWords,
        std::chrono::milliseconds timeout,
        bool* errorOccurred);

    static bool protectedIsByteswappingRequired(avsCommon::utils::AudioFormat audioFormat);
};

void MockKeyWordDetector::sendKeyWordCallToObservers() {
    notifyKeyWordObservers(nullptr, "ALEXA", 0, 0);
}

void MockKeyWordDetector::sendStateChangeCallObservers(
    avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState state) {
    notifyKeyWordDetectorStateObservers(state);
}

ssize_t MockKeyWordDetector::protectedReadFromStream(
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    void* buf,
    size_t nWords,
    std::chrono::milliseconds timeout,
    bool* errorOccurred) {
    return readFromStream(reader, stream, buf, nWords, timeout, errorOccurred);
}

bool MockKeyWordDetector::protectedIsByteswappingRequired(avsCommon::utils::AudioFormat audioFormat) {
    return isByteswappingRequired(audioFormat);
}

class AbstractKeyWordDetectorTest : public ::testing::Test {
protected:
    std::shared_ptr<MockKeyWordDetector> m_detector;
    std::shared_ptr<MockKeyWordObserver> m_keyWordObserver;
    std::shared_ptr<MockStateObserver> m_stateObserver;
    std::shared_ptr<MockKeywordNotifier> m_keywordNotifier;
    std::shared_ptr<MockKeywordDetectorStateNotifier> m_keywordDetectorStateNotifier;
    std::shared_ptr<avsCommon::avs::AudioInputStream::Buffer> m_buffer;
    std::unique_ptr<avsCommon::avs::AudioInputStream> m_sds;
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> m_writer;
    std::unique_ptr<avsCommon::avs::AudioInputStream::Reader> m_reader;

    virtual void SetUp() {
        m_keywordNotifier = std::make_shared<MockKeywordNotifier>();
        m_keywordDetectorStateNotifier = std::make_shared<MockKeywordDetectorStateNotifier>();
        m_detector = std::make_shared<MockKeyWordDetector>(m_keywordNotifier, m_keywordDetectorStateNotifier);
        m_keyWordObserver = std::make_shared<MockKeyWordObserver>();
        m_stateObserver = std::make_shared<MockStateObserver>();

        m_buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(TEST_BUFFER_SIZE);
        m_sds = avsCommon::avs::AudioInputStream::create(m_buffer, 2, 1);
        ASSERT_TRUE(m_sds);
        m_writer = m_sds->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::BLOCKING);
        ASSERT_TRUE(m_writer);
        m_reader = m_sds->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING);
        ASSERT_TRUE(m_reader);
        // Make calls to KWDStateNotifier pass through to observer.
        ON_CALL(*m_keywordDetectorStateNotifier, notifyObservers(_))
            .WillByDefault(Invoke(
                [this](std::function<void(
                           const std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>&)>
                           notifyFn) { notifyFn(m_stateObserver); }));

        // Make calls to KWNotifier pass through to observer.
        ON_CALL(*m_keywordNotifier, notifyObservers(_))
            .WillByDefault(Invoke(
                [this](std::function<void(const std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>&)>
                           notifyFn) { notifyFn(m_keyWordObserver); }));

        // Initialize Detector State to Active from Closed.
        m_detector->sendStateChangeCallObservers(
            avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
    }
};

/**
 * Tests adding a Keyword Observer to the KWD.
 */
TEST_F(AbstractKeyWordDetectorTest, test_addKeyWordObserver) {
    EXPECT_CALL(*m_keywordNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordObserver(m_keyWordObserver);
}

/**
 * Tests Notifying a Keyword Observer.
 */
TEST_F(AbstractKeyWordDetectorTest, test_notifyKeyWordObserver) {
    // add kw observer
    EXPECT_CALL(*m_keywordNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordObserver(m_keyWordObserver);

    EXPECT_CALL(*m_keywordNotifier, notifyObservers(_)).Times(1);
    EXPECT_CALL(*m_keyWordObserver, onKeyWordDetected(_, _, _, _, _)).Times(1);
    m_detector->sendKeyWordCallToObservers();
}

/**
 * Tests removing a Keyword Observer to the KWD.
 */
TEST_F(AbstractKeyWordDetectorTest, test_removeKeyWordObserver) {
    EXPECT_CALL(*m_keywordNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordObserver(m_keyWordObserver);

    EXPECT_CALL(*m_keywordNotifier, removeObserver(_)).Times(1);
    m_detector->removeKeyWordObserver(m_keyWordObserver);
}

/**
 * Tests adding a Detector State Observer to the KWD.
 */
TEST_F(AbstractKeyWordDetectorTest, test_addStateObserver) {
    EXPECT_CALL(*m_keywordDetectorStateNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordDetectorStateObserver(m_stateObserver);
}

/**
 * Tests notifying a KeywordDetectorStateObserver.
 */
TEST_F(AbstractKeyWordDetectorTest, test_notifyStateObserver) {
    EXPECT_CALL(*m_keywordDetectorStateNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordDetectorStateObserver(m_stateObserver);

    EXPECT_CALL(*m_keywordDetectorStateNotifier, notifyObservers(_)).Times(1);
    EXPECT_CALL(
        *m_stateObserver,
        onStateChanged(
            avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED))
        .Times(1);
    m_detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
}

/**
 * Tests removing a Detector State Observer to the KWD.
 */
TEST_F(AbstractKeyWordDetectorTest, test_removeStateObserver) {
    EXPECT_CALL(*m_keywordDetectorStateNotifier, addObserver(_)).Times(1);
    m_detector->addKeyWordDetectorStateObserver(m_stateObserver);

    EXPECT_CALL(*m_keywordDetectorStateNotifier, removeObserver(_)).Times(1);
    m_detector->removeKeyWordDetectorStateObserver(m_stateObserver);
}

/**
 * Tests that Detector State Observers aren't notified if there is no change in state.
 */
TEST_F(AbstractKeyWordDetectorTest, test_observersDontGetNotifiedOfSameStateTwice) {
    m_detector->addKeyWordDetectorStateObserver(m_stateObserver);

    EXPECT_CALL(*m_keywordDetectorStateNotifier, notifyObservers(_)).Times(1);
    EXPECT_CALL(
        *m_stateObserver,
        onStateChanged(
            avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED))
        .Times(1);
    m_detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);

    EXPECT_CALL(*m_keywordDetectorStateNotifier, notifyObservers(_)).Times(0);
    EXPECT_CALL(*m_stateObserver, onStateChanged(_)).Times(0);
    m_detector->sendStateChangeCallObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
}

/**
 * Tests if byte swapping of the audio stream is required by comparing format endianness to platform endianness.
 */
TEST_F(AbstractKeyWordDetectorTest, test_isByteSwappingRequired) {
    int num = 1;
    char* firstBytePtr = reinterpret_cast<char*>(&num);
    avsCommon::utils::AudioFormat audioFormat;

    if (*firstBytePtr == 1) {
        // Test Platform is little endian.
        audioFormat.endianness = avsCommon::utils::AudioFormat::Endianness::LITTLE;
        EXPECT_FALSE(m_detector->protectedIsByteswappingRequired(audioFormat));
        audioFormat.endianness = avsCommon::utils::AudioFormat::Endianness::BIG;
        EXPECT_TRUE(m_detector->protectedIsByteswappingRequired(audioFormat));
    } else {
        // Test Platform is big endian.
        audioFormat.endianness = avsCommon::utils::AudioFormat::Endianness::LITTLE;
        EXPECT_TRUE(m_detector->protectedIsByteswappingRequired(audioFormat));
        audioFormat.endianness = avsCommon::utils::AudioFormat::Endianness::BIG;
        EXPECT_FALSE(m_detector->protectedIsByteswappingRequired(audioFormat));
    }
}

/**
 * Tests that KWD is able to read from stream successfully.
 */
TEST_F(AbstractKeyWordDetectorTest, test_readFromStreamSuccessful) {
    // Write random data into the m_sds.
    std::vector<int16_t> randomData(50, 0);
    m_writer->write(randomData.data(), randomData.size());

    // Attempt to read from stream with no errors occuring
    bool errorOccurred = false;
    EXPECT_EQ(
        WORDS_TO_READ,
        m_detector->protectedReadFromStream(
            std::move(m_reader), std::move(m_sds), m_buffer->data(), WORDS_TO_READ, TIMEOUT, &errorOccurred));
    EXPECT_FALSE(errorOccurred);
}

/**
 * Test reading from stream while the stream is closed.
 */
TEST_F(AbstractKeyWordDetectorTest, test_readFromStreamWhileStreamClosed) {
    m_reader->close();
    // Attempt to read a word from the closed stream.
    // Expect that zero words are read and that an error has occured.
    // Expect that state observers are notified of detector state change.
    bool errorOccurred = false;
    EXPECT_CALL(*m_keywordDetectorStateNotifier, notifyObservers(_)).Times(1);
    EXPECT_CALL(
        *m_stateObserver,
        onStateChanged(
            avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED))
        .Times(1);
    EXPECT_EQ(
        ZERO_WORDS_READ,
        m_detector->protectedReadFromStream(
            std::move(m_reader), std::move(m_sds), m_buffer->data(), WORDS_TO_READ, TIMEOUT, &errorOccurred));
    EXPECT_TRUE(errorOccurred);
}

/**
 * Test reading from stream when the m_buffer is overrun.
 */
TEST_F(AbstractKeyWordDetectorTest, test_readFromStreamBufferOverrun) {
    // Create Reader and Writers for test
    auto buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(TEST_BUFFER_SIZE);
    auto sds = avsCommon::avs::AudioInputStream::create(buffer, 2, 1);
    ASSERT_TRUE(sds);
    auto writer = sds->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
    ASSERT_TRUE(writer);
    auto reader = sds->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::NONBLOCKING);
    ASSERT_TRUE(reader);

    // Write to buffer twice to overrun it.
    std::vector<int16_t> randomData(TEST_BUFFER_SIZE, 0);
    writer->write(randomData.data(), randomData.size());
    writer->write(randomData.data(), randomData.size());

    bool errorOccurred = false;
    EXPECT_EQ(
        avsCommon::avs::AudioInputStream::Reader::Error::OVERRUN,
        m_detector->protectedReadFromStream(
            std::move(reader), std::move(sds), buffer->data(), WORDS_TO_READ, TIMEOUT, &errorOccurred));
    EXPECT_FALSE(errorOccurred);
}

/**
 * Test reading from stream times out.
 */
TEST_F(AbstractKeyWordDetectorTest, test_readFromStreamTimedOut) {
    bool errorOccurred = false;

    EXPECT_EQ(
        avsCommon::avs::AudioInputStream::Reader::Error::TIMEDOUT,
        m_detector->protectedReadFromStream(
            std::move(m_reader), std::move(m_sds), m_buffer->data(), WORDS_TO_READ, TIMEOUT, &errorOccurred));
    EXPECT_FALSE(errorOccurred);
}

}  // namespace test
}  // namespace acsdkKWDImplementations
}  // namespace alexaClientSDK
