/*
 * DirectiveSequencerTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <chrono>
#include <future>
#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ADSL/DirectiveSequencer.h"

#include "ACL/AttachmentManagerInterface.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace avsCommon;
using namespace acl;

/// Macro to wait for a future using the default done timeout and assert that the timeout was not reached.
#define ASSERT_WAIT_FOR_TIMEOUT(future) ASSERT_EQ(future.wait_for(DEFAULT_DONE_TIMEOUT_MS), std::future_status::ready)

/// Default amount of time taken to handle a directive.
static const std::chrono::milliseconds DEFAULT_HANDLING_TIME_MS(0);

/// Long amount of time for handling a directive to allow other things to happen (we should not reach this).
static const std::chrono::milliseconds LONG_HANDLING_TIME_MS(30000);

/// Timeout used when waiting for tests to complete (we should not reach this).
static const std::chrono::milliseconds DEFAULT_DONE_TIMEOUT_MS(60000);

/// Namespace for Test only directives.
static const std::string NAMESPACE_TEST("Test");

/// Namespace for Speaker directives.
static const std::string NAMESPACE_SPEAKER("Speaker");

/// Namespace for SpeechSynthesizer directives.
static const std::string NAMESPACE_SPEECH_SYNTHESIZER("SpeechSynthesizer");

/// Namespace for AudioPlayer directives.
static const std::string NAMESPACE_AUDIO_PLAYER("AudioPlayer");

/// Name for Test directive used to terminate tests.
static const std::string NAME_DONE("Done");

/// Name for Speaker::setVolume Directives.
static const std::string NAME_SET_VOLUME("SetVolume");

/// Name for SpeechSynthesizer::Speak directives.
static const std::string NAME_SPEAK("Speak");

/// Name for AutioPlayer::Play directives.
static const std::string NAME_PLAY("Play");

/// Name for Test::Blocking directives
static const std::string NAME_BLOCKING("Blocking");

/// Name for Test::Non-Blocking directives.
static const std::string NAME_NON_BLOCKING("Non-Blocking");

/// MessageId for Testing:Done directives used to terminate tests.
static const std::string MESSAGE_ID_DONE("Message_Done");

/// Generic messageId used for tests.
static const std::string MESSAGE_ID_0("Message_0");

/// Generic MessageId used for test.
static const std::string MESSAGE_ID_1("Message_1");

/// Generic MessageId used for tests.
static const std::string MESSAGE_ID_2("Message_2");

/// Default DialogRequestId for directives used to terminate tests.
static const std::string DIALOG_REQUEST_ID_DONE("DialogRequestId_Done");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_0("DialogRequestId_0");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_1("DialogRequestId_1");

/// An unparsed directive for test.
static const std::string UNPARSED_DIRECTIVE("unparsedDirectiveForTest");

/// A paylaod for test;
static const std::string PAYLOAD_TEST("payloadForTest");

/// Generic DialogRequestId used for tests.
static const std::string DIALOG_REQUEST_ID_2("DialogRequestId_2");

/**
 * MockDirectiveHandler used to verify DirectiveSequencer behavior.
 * NOTE: Each instance contains directive specific state, so it must only be be used to process a single directive.
 */
class MockDirectiveHandler : public DirectiveHandlerInterface {
public:

    /**
     * Create a MockDirectiveHandler.
     * @param handlingTimeMs The amount of time (in milliseconds) this handler takes to handle directives.
     * @return A new MockDirectiveHandler.
     */
    static std::shared_ptr<NiceMock<MockDirectiveHandler>> create(
            std::chrono::milliseconds handlingTimeMs = DEFAULT_HANDLING_TIME_MS);

    /**
     * Constructor.
     * @param handlingTimeMs The amount of time (in milliseconds) this handler takes to handle directives.
     */
    MockDirectiveHandler(std::chrono::milliseconds handlingTimeMs);

    /// Destructor.
    ~MockDirectiveHandler();

    /**
     * The functional part of mocking handleDirectiveImmediately().
     * @param directive The directive to handle.
     */
    void mockHandleDirectiveImmediately(std::shared_ptr<AVSDirective> directive);

    /**
     * The functional part of mocking preHandleDirective().
     * @param directive The directive to pre-handle.
     * @param result The result object to
     */
    void mockPreHandleDirective(
        std::shared_ptr<AVSDirective> directive,
        std::shared_ptr<DirectiveHandlerResultInterface> result);

    /**
     * The functional part of mocking handleDirective().
     * @param messageId The MessageId of the @c AVSDirective to handle.
     */
    void mockHandleDirective(const std::string& messageId);

    /**
     * The functional part of mocking cancelDirective().
     * @param directive The MessageId of the directive to cancel.
     */
    void mockCancelDirective(const std::string& messageId);

    /**
     * The functional part of mocking shutdown().
     */
    void mockShutdown();

    /**
     * Method for m_doHandleDirectiveThread.  Waits a specified time before reporting completion.
     * If cancelDirective() is called in the mean time, wake up and exit.
     * @param messageId The messageId to pass to the observer when reporting completion.
     */
    void doHandleDirective(const std::string& messageId);

    /**
     * Fail preHandleDirective() by calling onDirectiveError().
     * @param directive The directive to simulate the failure of.
     * @param result An object to receive the result of the handling operation.
     */
    void doPreHandlingFailed(
            std::shared_ptr<AVSDirective> directive,
            std::shared_ptr<DirectiveHandlerResultInterface> result);

    /**
     * Fail handleDirective() by calling onDirectiveError().
     * @param messageId The MessageId of the directive to simulate the failure of.
     */
    void doHandlingFailed(const std::string& messageId);

    /**
     * Common shutdown code.
     */
    void shutdownCommon();

    /**
     * Block until preHandleDirective() is called.
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if preHandeDirective() was called before the timeout.
     */
    bool waitUntilPreHandling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Block until handleDirective() is called.
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if handeDirective() was called before the timeout.
     */
    bool waitUntilHandling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Block until cancelDirective() is called.
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if cancelDirective() was called before the timeout.
     */
    bool waitUntilCanceling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Return whether shutdown() has been called.
     * @return whether shutdown() has been called.
     */
    bool isShuttingDown();

    /// The amount of time (in milliseconds) handling a directive will take.
    std::chrono::milliseconds m_handlingTimeMs;

    /// Object used to specify the result of handling a directive.
    std::shared_ptr<DirectiveHandlerResultInterface> m_result;

    /// Thread to perform handleDirective() asynchronously.
    std::thread m_doHandleDirectiveThread;

    /// Mutex to protect m_isShuttingDown.
    std::mutex m_mutex;

    /// Whether shutdown() has been called.
    bool m_isShuttingDown;

    /// condition_variable use to wake doHandlingDirectiveThread.
    std::condition_variable m_wakeNotifier;

    /// Promise fulfilled when preHandleDirective() begins.
    std::promise<void> m_preHandlingPromise;

    /// Future to notify when preHandleDirective() begins.
    std::future<void> m_preHandlingFuture;

    /// Promise fulfilled when handleDirective() begins.
    std::promise<void> m_handlingPromise;

    /// Future to notify when handleDirective() begins.
    std::future<void> m_handlingFuture;

    /// Promise fulfilled when cancelDirective() begins.
    std::promise<void> m_cancelingPromise;

    /// Future to notify when cancelDirective() begins.
    std::shared_future<void> m_cancelingFuture;

    MOCK_METHOD1(handleDirectiveImmediately, void(std::shared_ptr<AVSDirective>));
    MOCK_METHOD2(
            preHandleDirective,
            void(std::shared_ptr<AVSDirective>, std::shared_ptr<DirectiveHandlerResultInterface>));
    MOCK_METHOD1(handleDirective, void(const std::string&));
    MOCK_METHOD1(cancelDirective, void(const std::string&));
    MOCK_METHOD0(shutdown, void());
};

class MockAttachmentManager : public AttachmentManagerInterface {
public:
    MOCK_METHOD1(createAttachmentReader, std::future<std::shared_ptr<std::iostream>> (const std::string& attachmentId));
    MOCK_METHOD2(createAttachment, void (const std::string& attachmentId, std::shared_ptr<std::iostream> attachment));
    MOCK_METHOD1(releaseAttachment, void (const std::string& attachmentId));
};


std::shared_ptr<NiceMock<MockDirectiveHandler>> MockDirectiveHandler::create(std::chrono::milliseconds handlingTimeMs) {
    auto result = std::make_shared<NiceMock<MockDirectiveHandler>>(handlingTimeMs);
    ON_CALL(*result.get(), handleDirectiveImmediately(_)).WillByDefault(
            Invoke(result.get(), &MockDirectiveHandler::mockHandleDirectiveImmediately));
    ON_CALL(*result.get(), preHandleDirective(_, _)).WillByDefault(
            Invoke(result.get(), &MockDirectiveHandler::mockPreHandleDirective));
    ON_CALL(*result.get(), handleDirective(_)).WillByDefault(
            Invoke(result.get(), &MockDirectiveHandler::mockHandleDirective));
    ON_CALL(*result.get(), cancelDirective(_)).WillByDefault(
            Invoke(result.get(), &MockDirectiveHandler::mockCancelDirective));
    ON_CALL(*result.get(), shutdown()).WillByDefault(
            Invoke(result.get(), &MockDirectiveHandler::mockShutdown));
    return result;
}

MockDirectiveHandler::MockDirectiveHandler(std::chrono::milliseconds handlingTimeMs) :
        m_handlingTimeMs{handlingTimeMs},
        m_isShuttingDown{false},
        m_preHandlingPromise{},
        m_preHandlingFuture{m_preHandlingPromise.get_future()},
        m_handlingPromise{},
        m_handlingFuture{m_handlingPromise.get_future()},
        m_cancelingPromise{},
        m_cancelingFuture{m_cancelingPromise.get_future()} {
}

MockDirectiveHandler::~MockDirectiveHandler() {
}

void MockDirectiveHandler::mockHandleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ASSERT_FALSE(isShuttingDown());
    m_handlingPromise.set_value();
}

void MockDirectiveHandler::mockPreHandleDirective(
        std::shared_ptr<AVSDirective> directive,
        std::shared_ptr<DirectiveHandlerResultInterface> result) {
    ASSERT_FALSE(isShuttingDown());
    m_result = result;
    m_preHandlingPromise.set_value();
}

void MockDirectiveHandler::mockHandleDirective(const std::string& messageId) {
    ASSERT_FALSE(isShuttingDown());
    m_doHandleDirectiveThread = std::thread(&MockDirectiveHandler::doHandleDirective, this, messageId);
}

void MockDirectiveHandler::mockCancelDirective(const std::string& messageId) {
    ASSERT_FALSE(isShuttingDown());
    m_cancelingPromise.set_value();
    shutdownCommon();
}

void MockDirectiveHandler::mockShutdown() {
    ASSERT_TRUE(waitUntilCanceling(std::chrono::milliseconds(0)) || !isShuttingDown());
    shutdownCommon();
}

void MockDirectiveHandler::doHandleDirective(const std::string& messageId) {
    auto isShuttingDown = [this]() {
        return m_isShuttingDown;
    };
    m_handlingPromise.set_value();
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeNotifier.wait_for(lock, m_handlingTimeMs, isShuttingDown)) {
        m_result->setCompleted();
    }
}

void MockDirectiveHandler::doPreHandlingFailed(
        std::shared_ptr<AVSDirective> directive,
        std::shared_ptr<DirectiveHandlerResultInterface> result) {
    m_result = result;
    m_result->setFailed("doPreHandlingFailed()");
    m_preHandlingPromise.set_value();
}

void MockDirectiveHandler::doHandlingFailed(const std::string& messageId) {
    m_result->setFailed("doHandlingFailed()");
    m_handlingPromise.set_value();
}

void MockDirectiveHandler::shutdownCommon() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isShuttingDown = true;
        m_wakeNotifier.notify_all();
    }
    if (m_doHandleDirectiveThread.joinable()) {
        m_doHandleDirectiveThread.join();
    }
}

bool MockDirectiveHandler::waitUntilPreHandling(std::chrono::milliseconds timeout) {
    return m_preHandlingFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockDirectiveHandler::waitUntilHandling(std::chrono::milliseconds timeout) {
    return m_handlingFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockDirectiveHandler::waitUntilCanceling(std::chrono::milliseconds timeout) {
    return m_cancelingFuture.wait_for(timeout) == std::future_status::ready;
}

bool MockDirectiveHandler::isShuttingDown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isShuttingDown;
}

/// DirectiveSequencerTest
class DirectiveSequencerTest : public ::testing::Test {
public:

    /// Constructor.
    DirectiveSequencerTest();

    /**
     * Setup the DirectiveSequencerTest.  Creates a DirectiveSequencer and initialized it with a DirectiveHandler
     * that allows waiting for a final directive to be processed.
     */
    void SetUp() override;

    /**
     * TearDown the DirectiveSequencerTest.  Sends a Test.Done directive with the current dialogRequestId and
     * waits for it to be handled before allowing the test to complete.
     */
    void TearDown() override;

    /**
     * SetDialogRequestId().  Tests should use this method rather than m_sequencer->setDialogRequestId() so
     * that @c TearDown() knows which @c DialogRequestId to use for the Test::Done directive.
     * @param dialogRequestId The new value for DialogRequestId.
     */
    void setDialogRequestId(std::string dialogRequestId);

    /**
     * Method to invoke when handleDirective() is invoked for the Test::Done directive.
     * @param messageId The message ID of a directive previously passed to preHandleDirective().
     */
    void notifyDone(std::string messageId);

    /// The DialogRequestId to use for the Test::Done directive.
    std::string m_latestDialogRequestId;

    /// Handler to invoke for the Test::Done directive.
    std::shared_ptr<MockDirectiveHandler> m_doneHandler;

    /// The DirectiveSequencer to test.
    std::shared_ptr<DirectiveSequencer> m_sequencer;

    /// Promise fulfilled when the test is done.
    std::promise<void> m_donePromise;

    /// Future used to notify when the test is done.
    std::future<void> m_doneFuture;

    /// Mock object of AttachmentManagerInterface
    std::shared_ptr<acl::AttachmentManagerInterface> m_mockAttachmentManager;
};

DirectiveSequencerTest::DirectiveSequencerTest() : m_donePromise{}, m_doneFuture{m_donePromise.get_future()} {
}

void DirectiveSequencerTest::SetUp() {
    m_doneHandler = MockDirectiveHandler::create();
    m_mockAttachmentManager = std::make_shared<MockAttachmentManager>();
    m_sequencer = DirectiveSequencer::create(nullptr);
    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_TEST, NAME_DONE}, {m_doneHandler, BlockingPolicy::BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
}

void DirectiveSequencerTest::TearDown() {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_DONE, MESSAGE_ID_DONE,
                                                               DIALOG_REQUEST_ID_DONE);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST,
                                                                   m_mockAttachmentManager);
    EXPECT_CALL(*(m_doneHandler.get()), handleDirective(MESSAGE_ID_DONE)).WillOnce(
            Invoke(this, &DirectiveSequencerTest::notifyDone));
    m_sequencer->setDialogRequestId(DIALOG_REQUEST_ID_DONE);
    m_sequencer->onDirective(directive);
    m_doneFuture.wait_for(DEFAULT_DONE_TIMEOUT_MS);
    m_sequencer->shutdown();
}

void DirectiveSequencerTest::setDialogRequestId(std::string dialogRequestId) {
    m_latestDialogRequestId = dialogRequestId;
    m_sequencer->setDialogRequestId(m_latestDialogRequestId);
}

void DirectiveSequencerTest::notifyDone(std::string messageId) {
    m_donePromise.set_value();
}

/**
 * Verify core DirectiveSequencerTest.  Expect a new non-null instance of m_sequencer.
 */
TEST_F(DirectiveSequencerTest, testCreateAndDoneTrigger) {
    ASSERT_TRUE(m_sequencer);
}

/**
 * Exercise sending a @c nullptr to @c onDirective.  Expect that false is returned.
 */
TEST_F(DirectiveSequencerTest, testNullptrDirective) {
    ASSERT_FALSE(m_sequencer->onDirective(nullptr));
}

/**
 * Send a directive with an empty DialogRequestId.
 * Expect a call to handleDirectiveImmediately().
 */
TEST_F(DirectiveSequencerTest, testEmptyDialogRequestId) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST,
                                                                   m_mockAttachmentManager);
    auto handler = MockDirectiveHandler::create();
    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(directive)).Times(1);
    EXPECT_CALL(*(handler.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler.get()), cancelDirective(_)).Times(0);
    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEAKER, NAME_SET_VOLUME}, {handler, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
}

/**
 * Set handlers (NON_BLOCKING) for two namespace,name pairs.  Then remove one and change the other.  Send directives
 * for each of the namespace,name pairs.  Expect that the directive with no mapping is not seen by a handler that
 * the one that still has a handler is handled.
 */
TEST_F(DirectiveSequencerTest, testRemovingAndChangingHandlers) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_0);
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_NON_BLOCKING, MESSAGE_ID_1);

    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);

    auto handler0 = MockDirectiveHandler::create();
    auto handler1 = MockDirectiveHandler::create();

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive1)).Times(1);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler0.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEAKER, NAME_SET_VOLUME}, {handler0, BlockingPolicy::NON_BLOCKING}},
            {{NAMESPACE_TEST, NAME_NON_BLOCKING}, {handler1, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
    ready = m_sequencer->setDirectiveHandlers({
           {{NAMESPACE_SPEAKER, NAME_SET_VOLUME}, {nullptr, BlockingPolicy::NONE}},
            {{NAMESPACE_TEST, NAME_NON_BLOCKING}, {handler0, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
}

/**
 * Send a long running directive with an non-empty @c DialogRequestId and a BLOCKING policy. Expect a call to
 * @c preHandleDirective() and a call to @c handleDirective().  The @c AVSDirective is the cancelled, triggering
 * a call to cancelDirective() to close out the test.
 */
TEST_F(DirectiveSequencerTest, testBlockingDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                               DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    auto handler = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler.get()), preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*(handler.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler.get()), cancelDirective(_)).Times(1);
    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler, BlockingPolicy::BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler->waitUntilCanceling());
}

/**
 * Send a long running @c AVSDirective with an non-empty @c DialogRequestId and a @c BLOCKING policy followed by
 * a @c AVSDirective without a @c dialogRequestId.  Expect that the second directive gets handled immediately
 * and without waiting for the @c BLOCKING directive to complete.
 */
TEST_F(DirectiveSequencerTest, testBlockingThenNonDialogDirective) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(
            NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0, DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(
            UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST, m_mockAttachmentManager);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEAKER, NAME_SET_VOLUME, MESSAGE_ID_1);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(
            UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST, m_mockAttachmentManager);

    auto handler0 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    auto handler1 = MockDirectiveHandler::create();

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(1);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(_, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_SPEAKER, NAME_SET_VOLUME}, {handler1, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler1->waitUntilHandling());
    ASSERT_TRUE(handler0->waitUntilHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler0->waitUntilCanceling());
}

/**
 * Send a directive with an non-empty @c DialogRequestId and a @c BLOCKING policy, to an @c DirectiveHandler with a
 * long handling time.  Once the @c DirectiveHandler is handling the @c AVSDirective, set the @c DirectiveSequencer's
 * @c DialogRequestId (simulating an outgoing @c SpeechRecognizer request). Expect a call to
 * @c preHandleDirective(@cAVSDirective) a call to @c handleDirective(@c MessageId, @c DirectiveHandlingResult),
 * and a call to @c cancelDirective(@c MessageId).
 */
TEST_F(DirectiveSequencerTest, testBargeIn) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                               DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);

    auto handler = MockDirectiveHandler::create(std::chrono::milliseconds(LONG_HANDLING_TIME_MS));


    EXPECT_CALL(*(handler.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler.get()), preHandleDirective(directive, _)).Times(1);
    EXPECT_CALL(*(handler.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler.get()), cancelDirective(MESSAGE_ID_0)).Times(1);

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler, BlockingPolicy::BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);
    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive);
    ASSERT_TRUE(handler->waitUntilHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_1);
    ASSERT_TRUE(handler->waitUntilCanceling());
}

/**
 * Send an @c AVSDirective with an non-empty @c DialogRequestId and a @c BLOCKING policy followed by two
 * @c NON_BLOCKING @c AVSDirectives with the same @c DialogRequestId. Expect a call to
 * @c preHandleDirective(@c AVSDirective) and a call to @c handleDirective() for each @c AVSDirective.
 * Along the way we set the DialogRequestId to the same value to verify that that setting it to the
 * current value does not cancel queued directives.
 */
TEST_F(DirectiveSequencerTest, testBlockingThenNonBockingOnSameDialogId) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                               DIALOG_REQUEST_ID_0);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader2 = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_NON_BLOCKING, MESSAGE_ID_2,
                                                                DIALOG_REQUEST_ID_0);


    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);

    auto handler0 = MockDirectiveHandler::create();
    auto handler1 = MockDirectiveHandler::create();
    auto handler2 = MockDirectiveHandler::create();

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler1, BlockingPolicy::NON_BLOCKING}},
            {{NAMESPACE_TEST, NAME_NON_BLOCKING}, {handler2, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(1);
    EXPECT_CALL(*(handler1.get()), cancelDirective(_)).Times(0);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(_)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(_)).Times(0);

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive2);
    setDialogRequestId(DIALOG_REQUEST_ID_0);
    ASSERT_TRUE(handler2->waitUntilHandling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING @c AVSDirective with the same
 * @c DialogRequestId.  Once the first @c AVSDirective is being handled and the second @c AVSDirective is being
 * preHandled send a new @c AVDirective with a different @c DialogRequestId to simulate 'barge-in'. Expect that
 * the first two directives will be cancelled and the third one will be handled (and then cancelled at the
 * end by setting the dialogRequestId to close out the test).
 */
TEST_F(DirectiveSequencerTest, testThatBargeInDropsSubsequentDirectives) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader2 = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_BLOCKING, MESSAGE_ID_2,
                                                                DIALOG_REQUEST_ID_1);

    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);


    auto handler0 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    auto handler1 = MockDirectiveHandler::create();
    auto handler2 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler1, BlockingPolicy::NON_BLOCKING}},
            {{NAMESPACE_TEST, NAME_BLOCKING}, {handler2, BlockingPolicy::BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(1);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(directive2)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(MESSAGE_ID_2)).Times(1);

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
    ASSERT_TRUE(handler1->waitUntilPreHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_1);
    m_sequencer->onDirective(directive2);
    ASSERT_TRUE(handler2->waitUntilHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_2);
    ASSERT_TRUE(handler2->waitUntilCanceling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING @c AVSDirective with the same
 * @c DialogRequestId. When the first @c AVSDirective is preHandled, report a failure via @c setFailed().
 * Expect that the first @c AVSDirective will not be cancelled and that the second @c AVSDirective will be dropped
 * entirely.
 */
TEST_F(DirectiveSequencerTest, testPreHandleDirectiveError) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1,
                                                                DIALOG_REQUEST_ID_0);

    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);

    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);



    auto handler0 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    auto handler1 = MockDirectiveHandler::create();

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler1, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).WillOnce(
            Invoke(handler0.get(), &MockDirectiveHandler::doPreHandlingFailed));
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(0);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(0);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(0);

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilPreHandling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING directive with the same
 * @c DialogRequestId.  When the first @c AVSDirective is handled, report a failure via @c setFailed().
 * Expect that the first @c AVSDirective will not be cancelled and that the second @c AVSDirective may be
 * dropped before @c preHandleDirective() is called, and that if not, it will be cancelled.
 */
TEST_F(DirectiveSequencerTest, testHandleDirectiveError) {

    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1,
                                                                DIALOG_REQUEST_ID_0);
    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);

    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);


    auto handler0 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    auto handler1 = MockDirectiveHandler::create();

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler1, BlockingPolicy::NON_BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).WillOnce(
            Invoke(handler0.get(), &MockDirectiveHandler::doHandlingFailed));
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(0);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(AtMost(1));
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);;
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(AtMost(1));

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
}

/**
 * Send a long-running @c BLOCKING @c AVSDirective followed by a @c NON_BLOCKING directive with the same
 * @c DialogRequestId.  Once they have reached the handling and preHandling stages, respectively, call
 * setDirectiveHandlers(), swapping the handlers and policies for the two outstanding @c AVSDirectives.
 * Expect that both of the outstanding @c AVSDirectives will be cancelled.  Then send an @c BLOCKING
 * @c AVSDirective whose handler mapping has not changed.  Expect that that last @c AVSDirective will
 * be preHandled and handled.  The last @c AVSDirectiveis long running and in the end is cancelled to
 * close out the test.

 */
TEST_F(DirectiveSequencerTest, testSetDirectiveHandlersWhileHandlingDirectives) {
    auto avsMessageHeader0 = std::make_shared<AVSMessageHeader>(NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK, MESSAGE_ID_0,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(NAMESPACE_AUDIO_PLAYER, NAME_PLAY, MESSAGE_ID_1,
                                                                DIALOG_REQUEST_ID_0);

    auto avsMessageHeader2 = std::make_shared<AVSMessageHeader>(NAMESPACE_TEST, NAME_BLOCKING, MESSAGE_ID_2,
                                                                DIALOG_REQUEST_ID_1);

    std::shared_ptr<AVSDirective> directive0 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader0, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive1 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader1, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);
    std::shared_ptr<AVSDirective> directive2 = AVSDirective::create(UNPARSED_DIRECTIVE, avsMessageHeader2, PAYLOAD_TEST,
                                                                    m_mockAttachmentManager);


    auto handler0 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);
    auto handler1 = MockDirectiveHandler::create();
    auto handler2 = MockDirectiveHandler::create(LONG_HANDLING_TIME_MS);

    auto ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler1, BlockingPolicy::NON_BLOCKING}},
            {{NAMESPACE_TEST, NAME_BLOCKING}, {handler2, BlockingPolicy::BLOCKING}}
    });
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    EXPECT_CALL(*(handler0.get()), handleDirectiveImmediately(directive0)).Times(0);
    EXPECT_CALL(*(handler0.get()), preHandleDirective(directive0, _)).Times(1);
    EXPECT_CALL(*(handler0.get()), handleDirective(MESSAGE_ID_0)).Times(1);
    EXPECT_CALL(*(handler0.get()), cancelDirective(MESSAGE_ID_0)).Times(1);

    EXPECT_CALL(*(handler1.get()), handleDirectiveImmediately(directive1)).Times(0);
    EXPECT_CALL(*(handler1.get()), preHandleDirective(directive1, _)).Times(1);
    EXPECT_CALL(*(handler1.get()), handleDirective(MESSAGE_ID_1)).Times(0);
    EXPECT_CALL(*(handler1.get()), cancelDirective(MESSAGE_ID_1)).Times(1);

    EXPECT_CALL(*(handler2.get()), handleDirectiveImmediately(directive2)).Times(0);
    EXPECT_CALL(*(handler2.get()), preHandleDirective(directive2, _)).Times(1);
    EXPECT_CALL(*(handler2.get()), handleDirective(MESSAGE_ID_2)).Times(1);
    EXPECT_CALL(*(handler2.get()), cancelDirective(MESSAGE_ID_2)).Times(1);

    setDialogRequestId(DIALOG_REQUEST_ID_0);
    m_sequencer->onDirective(directive0);
    m_sequencer->onDirective(directive1);
    ASSERT_TRUE(handler0->waitUntilHandling());
    ASSERT_TRUE(handler1->waitUntilPreHandling());

    ready = m_sequencer->setDirectiveHandlers({
            {{NAMESPACE_SPEECH_SYNTHESIZER, NAME_SPEAK}, {handler1, BlockingPolicy::NON_BLOCKING}},
            {{NAMESPACE_AUDIO_PLAYER, NAME_PLAY}, {handler0, BlockingPolicy::BLOCKING}},
            {{NAMESPACE_TEST, NAME_BLOCKING}, {handler2, BlockingPolicy::BLOCKING}}
    });

    ASSERT_TRUE(handler1->waitUntilCanceling());
    ASSERT_WAIT_FOR_TIMEOUT(ready);

    setDialogRequestId(DIALOG_REQUEST_ID_1);
    m_sequencer->onDirective(directive2);
    ASSERT_TRUE(handler2->waitUntilHandling());
    setDialogRequestId(DIALOG_REQUEST_ID_2);
    ASSERT_TRUE(handler2->waitUntilCanceling());
}

} // namespace test
} // namespace adsl
} // namespace alexaClientSDK
