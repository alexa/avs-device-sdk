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

#ifndef ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCKDIRECTIVEHANDLER_H_
#define ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCKDIRECTIVEHANDLER_H_

#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/AVS/NamespaceAndName.h>

namespace alexaClientSDK {
namespace adsl {
namespace test {

/**
 * gmock does not fully support C++11's move only semantics.  Replaces the use of unique_ptr in
 * DirectiveHandlerInterface with shared_ptr so that methods using unique_ptr can be mocked.
 */
class DirectiveHandlerMockAdapter : public avsCommon::sdkInterfaces::DirectiveHandlerInterface {
public:
    void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) override;

    /**
     * Variant of preHandleDirective taking a shared_ptr instead of a unique_ptr.
     *
     * @param directive The @c AVSDirective to be (pre) handled.
     * @param result The object to receive completion/failure notifications.
     */
    virtual void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) = 0;
};

/**
 * MockDirectiveHandler.  This mock sports the following functionality:
 *   - Spins up a thread when handleDirective() is invoked.
 *   - Triggers early termination of the handling thread when cancelDirective() is invoked.
 *   - Makes appropriate callbacks to DirectiveHandlerResultInterface instances passed bin by preHandle().
 *   - Provides functions to dynamically trigger completion and failure.
 *   - Provides methods to wait for the DirectiveHandlerInterface methods to be called.
 *
 * @note Each instance contains directive specific state, so it must only be be used to process a single directive.
 */
class MockDirectiveHandler : public DirectiveHandlerMockAdapter {
public:
    /**
     * Create a MockDirectiveHandler.
     *
     * @param config The @c avsCommon::avs::DirectiveHandlerConfiguration of the handler.
     * @param handlingTimeMs The amount of time (in milliseconds) this handler takes to handle directives.
     * @return A new MockDirectiveHandler.
     */
    static std::shared_ptr<testing::NiceMock<MockDirectiveHandler>> create(
        avsCommon::avs::DirectiveHandlerConfiguration config,
        std::chrono::milliseconds handlingTimeMs = DEFAULT_HANDLING_TIME_MS);

    /**
     * Constructor.
     *
     * @param handlingTimeMs The amount of time (in milliseconds) this handler takes to handle directives.
     */
    MockDirectiveHandler(
        avsCommon::avs::DirectiveHandlerConfiguration config,
        std::chrono::milliseconds handlingTimeMs);

    /// Destructor.
    ~MockDirectiveHandler();

    /**
     * The functional part of mocking handleDirectiveImmediately().
     *
     * @param directive The directive to handle.
     */
    void mockHandleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * The functional part of mocking preHandleDirective().
     *
     * @param directive The directive to pre-handle.
     * @param result The result object to
     */
    void mockPreHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result);

    /**
     * The functional part of mocking handleDirective().
     *
     * @param messageId The MessageId of the @c AVSDirective to handle.
     * @return Whether handling the directive has begun.
     */
    bool mockHandleDirective(const std::string& messageId);

    /**
     * The functional part of mocking cancelDirective().
     *
     * @param directive The MessageId of the directive to cancel.
     */
    void mockCancelDirective(const std::string& messageId);

    /**
     * The functional part of mocking onDeregistered().
     */
    void mockOnDeregistered();

    /**
     * Method for m_doHandleDirectiveThread.  Waits a specified time before reporting completion.
     * If cancelDirective() is called in the mean time, wake up and exit.
     *
     * @param messageId The messageId to pass to the observer when reporting completion.
     */
    void doHandleDirective(const std::string& messageId);

    /**
     * Fail preHandleDirective() by calling onDirectiveError().
     *
     * @param directive The directive to simulate the failure of.
     * @param result An object to receive the result of the handling operation.
     */
    void doPreHandlingFailed(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result);

    /**
     * Trigger completion of handling.
     */
    void doHandlingCompleted();

    /**
     * Fail handleDirective() by calling onDirectiveError().
     *
     * @param messageId The MessageId of the directive to simulate the failure of.
     */
    bool doHandlingFailed(const std::string& messageId);

    void shutdown();

    /**
     * Block until preHandleDirective() is called.
     *
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if preHandleDirective() was called before the timeout.
     */
    bool waitUntilPreHandling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Block until handleDirective() is called.
     *
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if handleDirective() was called before the timeout.
     */
    bool waitUntilHandling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Block until cancelDirective() is called.
     *
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if cancelDirective() was called before the timeout.
     */
    bool waitUntilCanceling(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /**
     * Block until completed() result is specified.
     *
     * @param timeout The max amount of time to wait (in milliseconds).
     * @return @c true if setCompleted() was called before the timeout.
     */
    bool waitUntilCompleted(std::chrono::milliseconds timeout = DEFAULT_DONE_TIMEOUT_MS);

    /// The amount of time (in milliseconds) handling a directive will take.
    std::chrono::milliseconds m_handlingTimeMs;

    /// Object used to specify the result of handling a directive.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> m_result;

    /// The @c AVSDirective (if any) being handled.
    std::shared_ptr<avsCommon::avs::AVSDirective> m_directive;

    /// Thread to perform handleDirective() asynchronously.
    std::thread m_doHandleDirectiveThread;

    /// Mutex to protect m_isShuttingDown.
    std::mutex m_mutex;

    /// condition_variable use to wake doHandlingDirectiveThread.
    std::condition_variable m_wakeNotifier;

    /// Whether or not handling completed.
    bool m_isCompleted;

    /// Whether or not this handler is shutting down.
    bool m_isShuttingDown;

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

    /// Promise fulfilled when a completed result is reported.
    std::promise<void> m_completedPromise;

    /// Future to notify when a completed result is reported.
    std::future<void> m_completedFuture;

    MOCK_METHOD1(handleDirectiveImmediately, void(std::shared_ptr<avsCommon::avs::AVSDirective>));
    MOCK_METHOD2(
        preHandleDirective,
        void(
            std::shared_ptr<avsCommon::avs::AVSDirective>,
            std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface>));
    MOCK_METHOD1(handleDirective, bool(const std::string&));
    MOCK_METHOD1(cancelDirective, void(const std::string&));
    MOCK_METHOD0(onDeregistered, void());
    MOCK_CONST_METHOD0(getConfiguration, avsCommon::avs::DirectiveHandlerConfiguration());

    /// Default amount of time taken to handle a directive (0).
    static const std::chrono::milliseconds DEFAULT_HANDLING_TIME_MS;

    /// Timeout used when waiting for tests to complete (we should not reach this).
    static const std::chrono::milliseconds DEFAULT_DONE_TIMEOUT_MS;
};

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCKDIRECTIVEHANDLER_H_
