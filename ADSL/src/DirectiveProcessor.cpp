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

#include <algorithm>
#include <iostream>
#include <sstream>

#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "ADSL/DirectiveProcessor.h"

/// String to identify log entries originating from this file.
static const std::string TAG("DirectiveProcessor");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

std::mutex DirectiveProcessor::m_handleMapMutex;
DirectiveProcessor::ProcessorHandle DirectiveProcessor::m_nextProcessorHandle = 0;
std::unordered_map<DirectiveProcessor::ProcessorHandle, DirectiveProcessor*> DirectiveProcessor::m_handleMap;

DirectiveProcessor::DirectiveProcessor(DirectiveRouter* directiveRouter) :
        m_directiveRouter{directiveRouter},
        m_isShuttingDown{false},
        m_isEnabled{true},
        m_isHandlingDirective{false} {
    std::lock_guard<std::mutex> lock(m_handleMapMutex);
    m_handle = ++m_nextProcessorHandle;
    m_handleMap[m_handle] = this;
    m_processingThread = std::thread(&DirectiveProcessor::processingLoop, this);
}

DirectiveProcessor::~DirectiveProcessor() {
    shutdown();
}

void DirectiveProcessor::setDialogRequestId(const std::string& dialogRequestId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    setDialogRequestIdLocked(dialogRequestId);
}

bool DirectiveProcessor::onDirective(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_ERROR(LX("onDirectiveFailed").d("action", "ignored").d("reason", "nullptrDirective"));
        return false;
    }
    std::lock_guard<std::mutex> onDirectiveLock(m_onDirectiveMutex);
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isShuttingDown || !m_isEnabled) {
        ACSDK_WARN(LX("onDirectiveFailed")
                       .d("messageId", directive->getMessageId())
                       .d("action", "ignored")
                       .d("reason", m_isShuttingDown ? "shuttingDown" : "disabled"));
        return false;
    }
    if (!directive->getDialogRequestId().empty() && directive->getDialogRequestId() != m_dialogRequestId) {
        ACSDK_INFO(LX("onDirective")
                       .d("messageId", directive->getMessageId())
                       .d("action", "dropped")
                       .d("reason", "dialogRequestIdDoesNotMatch")
                       .d("directivesDialogRequestId", directive->getDialogRequestId())
                       .d("dialogRequestId", m_dialogRequestId));
        return true;
    }
    m_directiveBeingPreHandled = directive;
    lock.unlock();
    auto handled = m_directiveRouter->preHandleDirective(
        directive, alexaClientSDK::avsCommon::utils::memory::make_unique<DirectiveHandlerResult>(m_handle, directive));
    lock.lock();
    if (m_directiveBeingPreHandled) {
        m_directiveBeingPreHandled.reset();
        if (handled) {
            m_handlingQueue.push_back(directive);
            m_wakeProcessingLoop.notify_one();
        }
    }

    return handled;
}

void DirectiveProcessor::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_handleMapMutex);
        m_handleMap.erase(m_handle);
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        queueAllDirectivesForCancellationLocked();
        m_isShuttingDown = true;
        m_wakeProcessingLoop.notify_one();
    }
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
}

void DirectiveProcessor::disable() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("disable"));
    queueAllDirectivesForCancellationLocked();
    m_isEnabled = false;
    m_wakeProcessingLoop.notify_one();
}

bool DirectiveProcessor::enable() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_isEnabled = true;
    return m_isEnabled;
}

DirectiveProcessor::DirectiveHandlerResult::DirectiveHandlerResult(
    DirectiveProcessor::ProcessorHandle processorHandle,
    std::shared_ptr<AVSDirective> directive) :
        m_processorHandle{processorHandle},
        m_directive{directive} {
}

void DirectiveProcessor::DirectiveHandlerResult::setCompleted() {
    std::lock_guard<std::mutex> lock(m_handleMapMutex);
    auto it = m_handleMap.find(m_processorHandle);
    if (it == m_handleMap.end()) {
        ACSDK_DEBUG(LX("setCompletedIgnored").d("reason", "directiveSequencerAlreadyShutDown"));
        return;
    }
    it->second->onHandlingCompleted(m_directive);
}

void DirectiveProcessor::DirectiveHandlerResult::setFailed(const std::string& description) {
    std::lock_guard<std::mutex> lock(m_handleMapMutex);
    auto it = m_handleMap.find(m_processorHandle);
    if (it == m_handleMap.end()) {
        ACSDK_DEBUG(LX("setFailedIgnored").d("reason", "directiveSequencerAlreadyShutDown"));
        return;
    }
    it->second->onHandlingFailed(m_directive, description);
}

void DirectiveProcessor::onHandlingCompleted(std::shared_ptr<AVSDirective> directive) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("onHandlingCompeted")
                    .d("messageId", directive->getMessageId())
                    .d("directiveBeingPreHandled",
                       m_directiveBeingPreHandled ? m_directiveBeingPreHandled->getMessageId() : "(nullptr)"));

    removeDirectiveLocked(directive);
}

void DirectiveProcessor::onHandlingFailed(std::shared_ptr<AVSDirective> directive, const std::string& description) {
    std::unique_lock<std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("onHandlingFailed")
                    .d("messageId", directive->getMessageId())
                    .d("directiveBeingPreHandled",
                       m_directiveBeingPreHandled ? m_directiveBeingPreHandled->getMessageId() : "(nullptr)")
                    .d("description", description));

    removeDirectiveLocked(directive);
    scrubDialogRequestIdLocked(directive->getDialogRequestId());
}

void DirectiveProcessor::removeDirectiveLocked(std::shared_ptr<AVSDirective> directive) {
    auto matches = [directive](std::shared_ptr<AVSDirective> item) { return item == directive; };

    m_cancelingQueue.erase(
        std::remove_if(m_cancelingQueue.begin(), m_cancelingQueue.end(), matches), m_cancelingQueue.end());

    if (matches(m_directiveBeingPreHandled)) {
        m_directiveBeingPreHandled.reset();
    }

    if (m_isHandlingDirective && !m_handlingQueue.empty() && matches(m_handlingQueue.front())) {
        m_isHandlingDirective = false;
        m_handlingQueue.pop_front();
    }

    m_handlingQueue.erase(
        std::remove_if(m_handlingQueue.begin(), m_handlingQueue.end(), matches), m_handlingQueue.end());

    if (!m_cancelingQueue.empty() || !m_handlingQueue.empty()) {
        m_wakeProcessingLoop.notify_one();
    }
}

void DirectiveProcessor::processingLoop() {
    auto wake = [this]() {
        return !m_cancelingQueue.empty() || (!m_handlingQueue.empty() && !m_isHandlingDirective) || m_isShuttingDown;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeProcessingLoop.wait(lock, wake);
        if (!processCancelingQueueLocked(lock) && !handleDirectiveLocked(lock) && m_isShuttingDown) {
            break;
        }
    }
}

bool DirectiveProcessor::processCancelingQueueLocked(std::unique_lock<std::mutex>& lock) {
    if (m_cancelingQueue.empty()) {
        return false;
    }
    std::deque<std::shared_ptr<avsCommon::avs::AVSDirective>> temp(std::move(m_cancelingQueue));
    lock.unlock();
    for (auto directive : temp) {
        m_directiveRouter->cancelDirective(directive);
    }
    lock.lock();
    return true;
}

bool DirectiveProcessor::handleDirectiveLocked(std::unique_lock<std::mutex>& lock) {
    if (m_handlingQueue.empty()) {
        return false;
    }
    if (m_isHandlingDirective) {
        return true;
    }
    auto directive = m_handlingQueue.front();
    m_isHandlingDirective = true;
    lock.unlock();
    auto policy = BlockingPolicy::NONE;
    auto handled = m_directiveRouter->handleDirective(directive, &policy);
    lock.lock();
    if (!handled || BlockingPolicy::BLOCKING != policy) {
        m_isHandlingDirective = false;
        if (!m_handlingQueue.empty() && m_handlingQueue.front() == directive) {
            m_handlingQueue.pop_front();
        } else if (!handled) {
            ACSDK_ERROR(LX("handlingDirectiveLockedFailed")
                            .d("expected", directive->getMessageId())
                            .d("front", m_handlingQueue.empty() ? "(empty)" : m_handlingQueue.front()->getMessageId())
                            .d("reason", "handlingQueueFrontChangedWithoutBeingHandled"));
        }
    }
    if (!handled) {
        scrubDialogRequestIdLocked(directive->getDialogRequestId());
    }
    return true;
}

void DirectiveProcessor::setDialogRequestIdLocked(const std::string& dialogRequestId) {
    if (dialogRequestId == m_dialogRequestId) {
        ACSDK_WARN(
            LX("setDialogRequestIdLockedIgnored").d("reason", "unchanged").d("dialogRequestId", dialogRequestId));
        return;
    }
    ACSDK_INFO(LX("setDialogRequestIdLocked").d("oldValue", m_dialogRequestId).d("newValue", dialogRequestId));
    scrubDialogRequestIdLocked(m_dialogRequestId);
    m_dialogRequestId = dialogRequestId;
}

void DirectiveProcessor::scrubDialogRequestIdLocked(const std::string& dialogRequestId) {
    if (dialogRequestId.empty()) {
        ACSDK_DEBUG(LX("scrubDialogRequestIdLocked").d("reason", "emptyDialogRequestId"));
        return;
    }

    ACSDK_DEBUG(LX("scrubDialogRequestIdLocked").d("dialogRequestId", dialogRequestId));

    bool changed = false;

    // If a matching directive is in the midst of a preHandleDirective() call (i.e. before the
    // directive is added to the m_handlingQueue) queue it for canceling instead.
    if (m_directiveBeingPreHandled) {
        auto id = m_directiveBeingPreHandled->getDialogRequestId();
        if (!id.empty() && id == dialogRequestId) {
            m_cancelingQueue.push_back(m_directiveBeingPreHandled);
            m_directiveBeingPreHandled.reset();
            changed = true;
        }
    }

    // If a mathcing directive in the midst of a handleDirective() call, reset m_isHandlingDirective
    // so we won't block processing subsequent directives.  This directive is already in m_handlingQueue
    // and will be moved to m_cancelingQueue, below.
    if (m_isHandlingDirective && !m_handlingQueue.empty()) {
        auto id = m_handlingQueue.front()->getDialogRequestId();
        if (!id.empty() && id == dialogRequestId) {
            m_isHandlingDirective = false;
            changed = true;
        }
    }

    // Filter matching directives from m_handlingQueue and put them in m_cancelingQueue.
    std::deque<std::shared_ptr<AVSDirective>> temp;
    for (auto directive : m_handlingQueue) {
        auto id = directive->getDialogRequestId();
        if (!id.empty() && id == dialogRequestId) {
            m_cancelingQueue.push_back(directive);
            changed = true;
        } else {
            temp.push_back(directive);
        }
    }
    std::swap(temp, m_handlingQueue);

    // If the dialogRequestId to scrub is the current value, reset the current value.
    if (dialogRequestId == m_dialogRequestId) {
        m_dialogRequestId.clear();
    }

    // If there were any changes, wake up the processing loop.
    if (changed) {
        m_wakeProcessingLoop.notify_one();
    }
}

void DirectiveProcessor::queueAllDirectivesForCancellationLocked() {
    m_dialogRequestId.clear();
    if (m_directiveBeingPreHandled) {
        m_handlingQueue.push_back(m_directiveBeingPreHandled);
        m_directiveBeingPreHandled.reset();
    }
    if (!m_handlingQueue.empty()) {
        m_cancelingQueue.insert(m_cancelingQueue.end(), m_handlingQueue.begin(), m_handlingQueue.end());
        m_handlingQueue.clear();
        m_wakeProcessingLoop.notify_one();
    }
    m_isHandlingDirective = false;
}

}  // namespace adsl
}  // namespace alexaClientSDK
