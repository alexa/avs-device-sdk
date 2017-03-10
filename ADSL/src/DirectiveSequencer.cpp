/*
* DirectiveSequencer.cpp
*
* Copyright 2017 Amazon.com, Inc. or its affiliates.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>

#include <AVSCommon/ExceptionEncountered.h>
#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>

#include "ADSL/DirectiveSequencer.h"

/// String used to identify log entries that originated from this file.
static const std::string TAG("DirectiveSequencer");

/// Macro to create a LogEntry in-line using the TAG for this file.
#define LX(event) ::alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

// TODO:  ACSDK-179 Remove this (and migrate invocations of this to ACSDK_<LEVEL> invocations.
#define ACSDK_LOG(expression)                                           \
    do {                                                                \
        ::alexaClientSDK::avsUtils::Logger::log(expression.c_str());    \
    } while (false)

namespace alexaClientSDK {

using namespace avsCommon;

namespace adsl {

std::shared_ptr<DirectiveSequencer> DirectiveSequencer::create(
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender) {
    std::shared_ptr<DirectiveSequencer> sequencer(new DirectiveSequencer(exceptionSender));
    sequencer->init(sequencer);
    return sequencer;
}

DirectiveSequencer::~DirectiveSequencer() {
    ACSDK_LOG(LX("~DirectiveSequencer()"));
    shutdown();
}

void DirectiveSequencer::shutdown() {
    ACSDK_LOG(LX("shutdown()"));
    setIsStopping();
    if (m_receivingThread.joinable()) {
        m_receivingThread.join();
    }
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
    clearDirectiveRouter();
}

std::future<bool>  DirectiveSequencer::setDirectiveHandlers(const DirectiveHandlerConfiguration& configuration) {
    ACSDK_LOG(LX("setDirectiveHandlers()"));
    auto newPromise = std::make_shared<std::promise<bool>>();
    std::lock_guard<std::mutex> lock(m_processingMutex);
    if (m_isStopping) {
        ACSDK_LOG(LX("setDirectiveHandlers()").d("m_isStopping", true).d("result", "ignored"));
        newPromise->set_value(false);
        return newPromise->get_future();
    }
    if (m_isSettingDirectiveHandlers) {
        newPromise->set_value(false);
        return newPromise->get_future();
    }

    m_setDirectiveHandlersPromise = newPromise;
    m_newDirectiveHandlerConfiguration = configuration;
    m_isSettingDirectiveHandlers = true;
    m_receivingNotifier.notify_one();
    m_dialogRequestId.clear();
    cancelPreHandledDirectivesLocked();
    return m_setDirectiveHandlersPromise->get_future();
}

void DirectiveSequencer::setDialogRequestId(const std::string& newDialogRequestId) {
    ACSDK_LOG(LX("setDialogRequestId()").d("newDialogRequestId", newDialogRequestId));
    if (m_isStopping) {
        ACSDK_LOG(LX("setDialogRequestId()").d("m_isStopping", true).d("result", "ignored"));
        return;
    }
    std::lock_guard<std::mutex> lock(m_processingMutex);
    if (newDialogRequestId != m_dialogRequestId) {
        m_dialogRequestId = newDialogRequestId;
        cancelPreHandledDirectivesLocked();
    } else {
        ACSDK_LOG(LX("setDialogRequestId() same as old value").d("result", "ignored"));
    }
}

bool DirectiveSequencer::onDirective(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_LOG(LX("onDirective():").d("directive", "(nullptr)").d("result", "ignored"));
        return false;
    }
    if (m_isStopping) {
        ACSDK_LOG(LX("onDirective()").d("directive", directive).d("m_isStopping", true).d("result", "ignored"));
        sendExceptionEncountered(directive, ExceptionErrorType::INTERNAL_ERROR, "DirectiveSequencer stopping.");
        return true;
    }
    std::lock_guard<std::mutex> lock(m_receivingMutex);
    auto handlerAndPolcy = m_directiveRouter.getDirectiveHandlerAndBlockingPolicy(directive);
    if (!handlerAndPolcy.first || BlockingPolicy::NONE == handlerAndPolcy.second) {
        ACSDK_LOG(LX("onDirective() no handler or policy for this directive.")
                .d("directive", directive).d("result", "ignored"));
        return false;
    }
    ACSDK_LOG(LX("onDirective()").d("directive", directive));
    m_receivingQueue.push_back(directive);
    m_receivingNotifier.notify_one();
    return true;
}

DirectiveSequencer::DirectiveHandlerResult::DirectiveHandlerResult(
        std::weak_ptr<DirectiveSequencer> directiveSequencer,
        std::shared_ptr<AVSDirective> directive,
        BlockingPolicy blockingPolicy) :
        m_directiveSequencer(directiveSequencer), m_directive(directive), m_blockingPolicy(blockingPolicy) {
}

void DirectiveSequencer::DirectiveHandlerResult::setCompleted() {
    if (m_blockingPolicy != BlockingPolicy::BLOCKING) {
        ACSDK_LOG(LX("DirectiveHandlerResult::setCompleted() for non-BLOCKING directive.")
                .d("messageId", m_directive->getMessageId()));
    }
    auto sequencer = m_directiveSequencer.lock();
    if (sequencer) {
        sequencer->onHandlingCompleted(m_directive);
    } else {
        ACSDK_LOG(LX("DirectiveHandlerResult::setCompleted() lock() failed")
                .d("messageId", m_directive->getMessageId()));
    }
}

void DirectiveSequencer::DirectiveHandlerResult::setFailed(const std::string& description) {
    auto sequencer = m_directiveSequencer.lock();
    if (sequencer) {
        sequencer->onHandlingFailed(m_directive, description);
    } else {
        ACSDK_LOG(LX("DirectiveHandlerResult::setFailed() lock() failed").d("messageId", m_directive->getMessageId()));
    }
}

DirectiveSequencer::DirectiveSequencer(
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender) :
        m_exceptionSender{exceptionSender},
        m_isStopping{false},
        m_handlingReceivedDirective{false},
        m_isSettingDirectiveHandlers{false} {
}

void DirectiveSequencer::init(std::weak_ptr<DirectiveSequencer> thisDirectiveSequencer) {
    m_thisDirectiveSequencer = thisDirectiveSequencer;
    m_receivingThread = std::thread(&DirectiveSequencer::receiveDirectivesLoop, this);
    m_processingThread = std::thread(&DirectiveSequencer::processDirectivesLoop, this);
}

void DirectiveSequencer::setIsStopping() {
    std::lock_guard<std::mutex> lock(m_receivingMutex);
    {
        // Although @c m_isStopping is atomic, that is not quite good enough to publish this modification to a
        // waiting thread.  We need to hold the lock as well.
        // @see http://en.cppreference.com/w/cpp/thread/condition_variable
        std::lock_guard<std::mutex> lock(m_processingMutex);
        m_isStopping = true;
    }
    m_receivingNotifier.notify_one();
    m_processingNotifier.notify_one();
}

void DirectiveSequencer::clearDirectiveRouter() {
    {
        std::lock_guard<std::mutex> lock(m_processingMutex);
        m_newDirectiveHandlerConfiguration.clear();
        m_isSettingDirectiveHandlers = false;
    }
    auto removedHandlers = m_directiveRouter.clear();
    ACSDK_LOG(LX("clearDirectiveRouter").d("action", "shutDownHandlers").d("count", removedHandlers.size()));
    for (auto handler : removedHandlers) {
        handler->shutdown();
    }
}

void DirectiveSequencer::onHandlingCompleted(std::shared_ptr<AVSDirective> directive) {
    removeDirectiveFromProcessing("onHandlingCompleted()", directive, false);
}

void DirectiveSequencer::onHandlingFailed(std::shared_ptr<AVSDirective> directive, const std::string& description) {
    avsUtils::logger::LogEntryStream text;
    text << "onHandlingFailed('" << description << "')";
    removeDirectiveFromProcessing(text.c_str(), directive, true);
}

void DirectiveSequencer::receiveDirectivesLoop() {
    while (!m_isStopping) {
        auto directive = receiveDirective();
        if (directive && shouldProcessDirective(directive)) {
            queueDirectiveForProcessing();
        }
    }
    ACSDK_LOG(LX("receiveDirectivesLoop(): stopping."));
}

std::shared_ptr<avsCommon::AVSDirective> DirectiveSequencer::receiveDirective() {
    auto wakeReceivingThread = [this]() {
        return m_isStopping || !m_receivingQueue.empty() || m_isSettingDirectiveHandlers;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_receivingMutex);
        m_receivingNotifier.wait(lock, wakeReceivingThread);
        if (m_receivingQueue.empty()) {
            lock.unlock();
            maybeSetDirectiveHandlers();
            lock.lock();
            if (m_isStopping && m_receivingQueue.empty()) {
                return nullptr;
            }
        } else {
            auto directive = m_receivingQueue.front();
            m_receivingQueue.pop_front();
            if (m_isSettingDirectiveHandlers) {
                ACSDK_LOG(LX("receiveDirective() while setting directive handlers.")
                        .d("messageId", directive->getMessageId()).d("result", "ignored"));
                sendExceptionEncountered(
                        directive, ExceptionErrorType::INTERNAL_ERROR, "Setting directive handlers.");
            } else {
                return directive;
            }
        }
    }
}

bool DirectiveSequencer::shouldProcessDirective(
        std::shared_ptr<avsCommon::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_processingMutex);
    auto handlerAndPolicy = m_directiveRouter.getDirectiveHandlerAndBlockingPolicy(directive);
    if (handlerAndPolicy.first) {
        if (directive->getDialogRequestId().empty()) {
            m_handlingReceivedDirective = true;
            lock.unlock();
            handlerAndPolicy.first->handleDirectiveImmediately(directive);
            lock.lock();
            m_handlingReceivedDirective = false;
            return false;
        } else if (directive->getDialogRequestId() == m_dialogRequestId) {
            m_directiveBeingPreHandled = directive;
            lock.unlock();
            handlerAndPolicy.first->preHandleDirective(directive, std::make_shared<DirectiveHandlerResult>(
                    m_thisDirectiveSequencer, directive, handlerAndPolicy.second));
            return true;
        } else {
            ACSDK_LOG(LX("shouldProcessDirective(): dialogRequestId does not match.")
                    .d("messageId", directive->getMessageId())
                    .d("m_dialogRequestId", m_dialogRequestId)
                    .d("dialogRequestId", directive->getDialogRequestId()));
            sendExceptionEncountered(
                    directive, ExceptionErrorType::INTERNAL_ERROR, "dialogRequestId does not match.");
        }
    } else {
        ACSDK_LOG(LX("shouldProcessDirective(): No handler for this directive (this should never happen).")
                .d("messageId", directive->getMessageId()));
        sendExceptionEncountered(
                directive, ExceptionErrorType::UNSUPPORTED_OPERATION, "No handler for this directive.");
    }
    return false;
}

void DirectiveSequencer::queueDirectiveForProcessing() {
    std::unique_lock<std::mutex> lock(m_processingMutex);
    if (!m_directiveBeingPreHandled) {
        return;
    }
    // Check that @c setDialogRequestId() was not called while @c m_processingMutex was unlocked when
    // @c preHandleDirective() was called.  If it was, queue the @c AVSDirective to be cancelled.
    if (m_directiveBeingPreHandled->getDialogRequestId() == m_dialogRequestId) {
        ACSDK_LOG(LX("queueDirectiveForProcessing(): push to m_handlingQueue")
                .d("messageId", m_directiveBeingPreHandled->getMessageId()));
        m_handlingQueue.push_back(m_directiveBeingPreHandled);
    } else {
        ACSDK_LOG(LX("queueDirectiveForProcessing(): push to m_cancelingQueue")
                .d("messageId", m_directiveBeingPreHandled->getMessageId()));
        m_cancellingQueue.push_back(m_directiveBeingPreHandled);
    }
    m_directiveBeingPreHandled.reset();
    m_processingNotifier.notify_one();
}

void DirectiveSequencer::processDirectivesLoop() {
    auto wakeProcessingThread = [this]() {
        return
                m_isStopping ||
                !m_cancellingQueue.empty() ||
                (!m_handlingQueue.empty() && !m_isHandlingDirective) ||
                m_isSettingDirectiveHandlers ||
                !m_removedHandlers.empty();
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_processingMutex);
        m_processingNotifier.wait(lock, wakeProcessingThread);
        shutdownRemovedHandlersLocked(lock);
        lock.unlock();
        if (shouldProcessingStop()) {
            break;
        }
        cancelDirectives();
        handleDirective();
    }
    ACSDK_LOG(LX("processDirectivesLoop(): stopping."));
}

void DirectiveSequencer::shutdownRemovedHandlersLocked(std::unique_lock<std::mutex>& lock) {
    while (!m_removedHandlers.empty()) {
        auto it = m_removedHandlers.begin();
        auto handler = *it;
        m_removedHandlers.erase(it);
        lock.unlock();
        handler->shutdown();
        lock.lock();
    }
}

bool DirectiveSequencer::shouldProcessingStop() {
    std::unique_lock<std::mutex> lock(m_processingMutex);
    if (m_isStopping) {
        if (!m_handlingQueue.empty()) {
            cancelPreHandledDirectivesLocked();
        }
        if (m_directiveBeingPreHandled) {
            ACSDK_LOG(LX("shouldProcessingStop(): push m_directiveBeingPreHandled to m_cancelingQueue")
                    .d("messageId", m_directiveBeingPreHandled->getMessageId()));
            m_cancellingQueue.push_back(m_directiveBeingPreHandled);
            m_directiveBeingPreHandled.reset();
        }
        if (m_cancellingQueue.empty()) {
            lock.unlock();
            maybeSetDirectiveHandlers();
            return true;
        }
    }
    return false;
}

void DirectiveSequencer::cancelDirectives() {
    std::unique_lock<std::mutex> lock(m_processingMutex);
    while (!m_cancellingQueue.empty()) {
        auto directive = m_cancellingQueue.front();
        m_cancellingQueue.pop_front();
        auto handler = m_directiveRouter.getDirectiveHandlerAndBlockingPolicy(directive).first;
        if (handler) {
            lock.unlock();
            handler->cancelDirective(directive->getMessageId());
            lock.lock();
        } else {
            ACSDK_LOG(LX("cancelDirective(): No handler for directive (this should never happen).")
                    .d("messageId", directive->getMessageId()));
            sendExceptionEncountered(
                    directive, ExceptionErrorType::UNSUPPORTED_OPERATION, "No handler for this directive.");
        }
    }
    if (m_handlingQueue.empty()) {
        lock.unlock();
        maybeSetDirectiveHandlers();
    }
}

void DirectiveSequencer::handleDirective() {
    std::unique_lock<std::mutex> lock(m_processingMutex);
    if (m_isHandlingDirective || m_handlingQueue.empty()) {
        return;
    }
    auto directive = m_handlingQueue.front();
    auto handlerAndPolicy = m_directiveRouter.getDirectiveHandlerAndBlockingPolicy(directive);
    if (handlerAndPolicy.first) {
        switch (handlerAndPolicy.second) {
            case BlockingPolicy::NON_BLOCKING:
                m_handlingQueue.pop_front();
                lock.unlock();
                handlerAndPolicy.first->handleDirective(directive->getMessageId());
                break;
            case BlockingPolicy::BLOCKING:
                m_isHandlingDirective = true;
                lock.unlock();
                handlerAndPolicy.first->handleDirective(directive->getMessageId());
                break;
            default:
                ACSDK_LOG(LX("processDirectivesLoop(): Unhandled BlockingPolicy: ")
                        .d("directive", directive).d("policy", handlerAndPolicy.second));
                m_handlingQueue.pop_front();
                lock.unlock();
                handlerAndPolicy.first->cancelDirective(directive->getMessageId());
                break;
        }
    } else {
        ACSDK_LOG(LX("handleDirective(): No handler for directive (this should never happen).")
                .d("messageId", directive->getMessageId()));
        sendExceptionEncountered(directive, ExceptionErrorType::INTERNAL_ERROR, "No handler for this directive.");
    }
}

void DirectiveSequencer::removeDirectiveFromProcessing(
        const std::string& caller, std::shared_ptr<AVSDirective> directive, bool cancelAllIfFound) {
    if (!directive) {
        ACSDK_LOG(LX("removeDirectiveFromProcessing() failed.").d("directive", "(nullptr)"));
        return;
    }

    auto matchFunction = [this, directive](std::shared_ptr<AVSDirective> element) {
        return element == directive;
    };

    std::lock_guard<std::mutex> lock(m_processingMutex);
    ACSDK_LOG(LX("removeDirectiveFromProcessing()")
            .d("caller", caller).d("messageId", directive->getMessageId()).d("cancelAllIfFound", cancelAllIfFound));
    auto it = std::find_if(m_handlingQueue.begin(), m_handlingQueue.end(), matchFunction);
    if (m_isHandlingDirective && it == m_handlingQueue.begin()) {
        m_isHandlingDirective = false;
        m_handlingQueue.pop_front();
        m_processingNotifier.notify_one();
    } else if (it != m_handlingQueue.end()) {
        m_handlingQueue.erase(it);
    } else if (m_directiveBeingPreHandled == directive) {
        m_directiveBeingPreHandled.reset();
    } else {
        it = std::find_if(m_cancellingQueue.begin(), m_cancellingQueue.end(), matchFunction);
        if (it != m_cancellingQueue.end()) {
            m_cancellingQueue.erase(it);
        } else {
            ACSDK_LOG(LX("removeDirectiveFromProcessing() directive NOT found."));
        }
        return;
    }
    if (cancelAllIfFound) {
        m_dialogRequestId.clear();
        cancelPreHandledDirectivesLocked();
    }
}

void DirectiveSequencer::cancelPreHandledDirectivesLocked() {
    m_cancellingQueue.insert(m_cancellingQueue.end(), m_handlingQueue.begin(), m_handlingQueue.end());
    m_handlingQueue.clear();
    m_isHandlingDirective = false;
    m_processingNotifier.notify_one();
}

void DirectiveSequencer::maybeSetDirectiveHandlers() {
    std::lock_guard<std::mutex> receivingLock(m_receivingMutex);
    if (m_receivingQueue.empty()) {
        std::lock_guard<std::mutex> processingLock(m_processingMutex);
        bool isDirectiveRouterInUse =
                m_handlingReceivedDirective ||
                m_directiveBeingPreHandled ||
                !m_cancellingQueue.empty() ||
                !m_handlingQueue.empty();

        if (m_isSettingDirectiveHandlers && !isDirectiveRouterInUse) {
            ACSDK_LOG(LX("maybeSetDirectiveHandlers() completed."));
            auto removedHandlers = m_directiveRouter.setDirectiveHandlers(m_newDirectiveHandlerConfiguration);
            m_removedHandlers.insert(removedHandlers.begin(), removedHandlers.end());
            m_processingNotifier.notify_one();
            m_newDirectiveHandlerConfiguration.clear();
            m_isSettingDirectiveHandlers = false;
            m_setDirectiveHandlersPromise->set_value(true);
            m_setDirectiveHandlersPromise.reset();
        }
    }
}

void DirectiveSequencer::sendExceptionEncountered(
        std::shared_ptr<avsCommon::AVSDirective> directive,
        avsCommon::ExceptionErrorType error,
        const std::string& message) {
    ACSDK_LOG(LX("sendExceptionEncountered()")
            .d("messageId", directive->getMessageId()).d("error", error).d("message", message));
    if (m_exceptionSender) {
        m_exceptionSender->sendExceptionEncountered(directive->getUnparsedDirective(), error, message);
    }
}

} // namespace directiveSequencer
} // namespace alexaClientSDK
