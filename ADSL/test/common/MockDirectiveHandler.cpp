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

#include "MockDirectiveHandler.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace adsl {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// Default amount of time taken to handle a directive.
const std::chrono::milliseconds MockDirectiveHandler::DEFAULT_HANDLING_TIME_MS(0);

/// Timeout used when waiting for tests to complete (we should not reach this).
const std::chrono::milliseconds MockDirectiveHandler::DEFAULT_DONE_TIMEOUT_MS(15000);

void DirectiveHandlerMockAdapter::preHandleDirective(
    std::shared_ptr<AVSDirective> directive,
    std::unique_ptr<DirectiveHandlerResultInterface> result) {
    std::shared_ptr<DirectiveHandlerResultInterface> temp(std::move(result));
    preHandleDirective(directive, temp);
}

std::shared_ptr<NiceMock<MockDirectiveHandler>> MockDirectiveHandler::create(
    DirectiveHandlerConfiguration config,
    std::chrono::milliseconds handlingTimeMs) {
    auto result = std::make_shared<NiceMock<MockDirectiveHandler>>(config, handlingTimeMs);
    ON_CALL(*result.get(), handleDirectiveImmediately(_))
        .WillByDefault(Invoke(result.get(), &MockDirectiveHandler::mockHandleDirectiveImmediately));
    ON_CALL(*result.get(), preHandleDirective(_, _))
        .WillByDefault(Invoke(result.get(), &MockDirectiveHandler::mockPreHandleDirective));
    ON_CALL(*result.get(), handleDirective(_))
        .WillByDefault(Invoke(result.get(), &MockDirectiveHandler::mockHandleDirective));
    ON_CALL(*result.get(), cancelDirective(_))
        .WillByDefault(Invoke(result.get(), &MockDirectiveHandler::mockCancelDirective));
    ON_CALL(*result.get(), onDeregistered())
        .WillByDefault(Invoke(result.get(), &MockDirectiveHandler::mockOnDeregistered));
    ON_CALL(*result.get(), getConfiguration()).WillByDefault(Return(config));
    return result;
}

MockDirectiveHandler::MockDirectiveHandler(
    DirectiveHandlerConfiguration config,
    std::chrono::milliseconds handlingTimeMs) :
        m_handlingTimeMs{handlingTimeMs},
        m_isCompleted{false},
        m_isShuttingDown{false},
        m_preHandlingPromise{},
        m_preHandlingFuture{m_preHandlingPromise.get_future()},
        m_handlingPromise{},
        m_handlingFuture{m_handlingPromise.get_future()},
        m_cancelingPromise{},
        m_cancelingFuture{m_cancelingPromise.get_future()},
        m_completedPromise{},
        m_completedFuture{m_completedPromise.get_future()} {
}

MockDirectiveHandler::~MockDirectiveHandler() {
    shutdown();
}

void MockDirectiveHandler::mockHandleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    m_handlingPromise.set_value();
}

void MockDirectiveHandler::mockPreHandleDirective(
    std::shared_ptr<AVSDirective> directive,
    std::shared_ptr<DirectiveHandlerResultInterface> result) {
    m_directive = directive;
    m_result = result;
    m_preHandlingPromise.set_value();
}

bool MockDirectiveHandler::mockHandleDirective(const std::string& messageId) {
    if (!m_directive || m_directive->getMessageId() != messageId) {
        return false;
    }
    m_doHandleDirectiveThread = std::thread(&MockDirectiveHandler::doHandleDirective, this, messageId);
    return true;
}

void MockDirectiveHandler::mockCancelDirective(const std::string& messageId) {
    if (!m_directive || m_directive->getMessageId() != messageId) {
        return;
    }
    m_cancelingPromise.set_value();
    shutdown();
}

void MockDirectiveHandler::doHandleDirective(const std::string& messageId) {
    auto wake = [this]() { return m_isCompleted || m_isShuttingDown; };
    m_handlingPromise.set_value();
    std::unique_lock<std::mutex> lock(m_mutex);
    m_wakeNotifier.wait_for(lock, m_handlingTimeMs, wake);
    if (!m_isShuttingDown) {
        m_isCompleted = true;
    }
    if (m_isCompleted) {
        m_result->setCompleted();
        m_completedPromise.set_value();
    }
}

void MockDirectiveHandler::mockOnDeregistered() {
}

void MockDirectiveHandler::doHandlingCompleted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isCompleted = true;
    m_wakeNotifier.notify_all();
}

void MockDirectiveHandler::doPreHandlingFailed(
    std::shared_ptr<AVSDirective> directive,
    std::shared_ptr<DirectiveHandlerResultInterface> result) {
    m_directive = directive;
    m_result = result;
    m_result->setFailed("doPreHandlingFailed()");
    m_preHandlingPromise.set_value();
}

bool MockDirectiveHandler::doHandlingFailed(const std::string& messageId) {
    if (!m_directive || m_directive->getMessageId() != messageId) {
        return false;
    }
    shutdown();
    m_result->setFailed("doHandlingFailed()");
    m_handlingPromise.set_value();
    return true;
}

void MockDirectiveHandler::shutdown() {
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

bool MockDirectiveHandler::waitUntilCompleted(std::chrono::milliseconds timeout) {
    return m_completedFuture.wait_for(timeout) == std::future_status::ready;
}

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK
