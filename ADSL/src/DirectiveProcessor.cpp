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
        m_isEnabled{true} {
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

    // TODO: ACSDK-2218
    // This additional check is made as a temporary solution to fix the problem with InteractionModel.NewDialogRequest
    // directives having a dialogRequestID in the payload even though it should be handled immediately. This additional
    // checking should be removed when a new InteractionModel version has been implemented which fixes the problem in
    // the directive.
    bool bypassDialogRequestIDCheck =
        directive->getName() == "NewDialogRequest" && directive->getNamespace() == "InteractionModel";

    if (bypassDialogRequestIDCheck) {
        ACSDK_INFO(LX("onDirective")
                       .d("action", "bypassingDialogRequestIdCheck")
                       .d("reason", "routinesDirectiveContainsDialogRequestId")
                       .d("directiveNamespace", "InteractionModel")
                       .d("directiveName", "NewDialogRequest"));
    } else if (!directive->getDialogRequestId().empty() && directive->getDialogRequestId() != m_dialogRequestId) {
        ACSDK_INFO(LX("onDirective")
                       .d("messageId", directive->getMessageId())
                       .d("action", "dropped")
                       .d("reason", "dialogRequestIdDoesNotMatch")
                       .d("directivesDialogRequestId", directive->getDialogRequestId())
                       .d("dialogRequestId", m_dialogRequestId));
        return true;
    }

    auto policy = m_directiveRouter->getPolicy(directive);

    m_directiveBeingPreHandled = directive;

    lock.unlock();
    auto preHandled = m_directiveRouter->preHandleDirective(
        directive, utils::memory::make_unique<DirectiveHandlerResult>(m_handle, directive));
    lock.lock();

    if (!m_directiveBeingPreHandled && preHandled) {
        return true;
    }

    m_directiveBeingPreHandled.reset();

    if (!preHandled) {
        return false;
    }

    auto item = std::make_pair(directive, policy);
    m_handlingQueue.push_back(item);
    m_wakeProcessingLoop.notify_one();

    return true;
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
    auto handlingMatches = [directive](std::pair<std::shared_ptr<AVSDirective>, BlockingPolicy> directiveAndPolicy) {
        return directiveAndPolicy.first == directive;
    };

    m_cancelingQueue.erase(
        std::remove_if(m_cancelingQueue.begin(), m_cancelingQueue.end(), matches), m_cancelingQueue.end());

    if (matches(m_directiveBeingPreHandled)) {
        m_directiveBeingPreHandled.reset();
    }

    m_handlingQueue.erase(
        std::remove_if(m_handlingQueue.begin(), m_handlingQueue.end(), handlingMatches), m_handlingQueue.end());

    if (m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO] &&
        matches(m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO])) {
        m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO].reset();
    }

    if (m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL] &&
        matches(m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL])) {
        m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL].reset();
    }

    if (!m_cancelingQueue.empty() || !m_handlingQueue.empty()) {
        m_wakeProcessingLoop.notify_one();
    }
}

void DirectiveProcessor::setDirectiveBeingHandledLocked(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const avsCommon::avs::BlockingPolicy policy) {
    if (policy.getMediums()[BlockingPolicy::Medium::AUDIO]) {
        m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO] = directive;
    }

    if (policy.getMediums()[BlockingPolicy::Medium::VISUAL]) {
        m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL] = directive;
    }
}

void DirectiveProcessor::clearDirectiveBeingHandledLocked(const avsCommon::avs::BlockingPolicy policy) {
    if ((policy.getMediums()[BlockingPolicy::Medium::AUDIO]) &&
        m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO]) {
        m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO].reset();
    }

    if ((policy.getMediums()[BlockingPolicy::Medium::VISUAL]) &&
        m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL]) {
        m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL].reset();
    }
}

std::set<std::shared_ptr<AVSDirective>> DirectiveProcessor::clearDirectiveBeingHandledLocked(
    std::function<bool(const std::shared_ptr<AVSDirective>&)> shouldClear) {
    std::set<std::shared_ptr<AVSDirective>> freed;

    auto directive = m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO];
    if (directive && shouldClear(directive)) {
        freed.insert(directive);
        m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO].reset();
    }

    directive = m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL];
    if (directive && shouldClear(directive)) {
        freed.insert(directive);
        m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL].reset();
    }

    return freed;
}

std::deque<DirectiveProcessor::DirectiveAndPolicy>::iterator DirectiveProcessor::getNextUnblockedDirectiveLocked() {
    // A medium is considered blocked if a previous blocking directive hasn't been completed yet.
    std::array<bool, BlockingPolicy::Medium::COUNT> blockedMediums;

    // Mark mediums used by blocking directives being handled as blocked.
    blockedMediums[BlockingPolicy::Medium::AUDIO] =
        (m_directivesBeingHandled[BlockingPolicy::Medium::AUDIO] != nullptr);
    blockedMediums[BlockingPolicy::Medium::VISUAL] =
        (m_directivesBeingHandled[BlockingPolicy::Medium::VISUAL] != nullptr);

    for (auto it = m_handlingQueue.begin(); it != m_handlingQueue.end(); it++) {
        auto currentUsingAudio = it->second.getMediums()[BlockingPolicy::Medium::AUDIO];
        auto currentUsingVisual = it->second.getMediums()[BlockingPolicy::Medium::VISUAL];

        if ((currentUsingAudio && blockedMediums[BlockingPolicy::Medium::AUDIO]) ||
            (currentUsingVisual && blockedMediums[BlockingPolicy::Medium::VISUAL])) {
            // if the current directive is blocking, block its Mediums.
            if (it->second.isBlocking()) {
                blockedMediums[BlockingPolicy::Medium::AUDIO] =
                    (blockedMediums[BlockingPolicy::Medium::AUDIO] || currentUsingAudio);
                blockedMediums[BlockingPolicy::Medium::VISUAL] =
                    (blockedMediums[BlockingPolicy::Medium::VISUAL] || currentUsingVisual);
            }
        } else {
            return it;
        }
    }

    return m_handlingQueue.end();
}

void DirectiveProcessor::processingLoop() {
    ACSDK_DEBUG9(LX("processingLoop"));

    auto haveUnblockedDirectivesToHandle = [this] {
        return getNextUnblockedDirectiveLocked() != m_handlingQueue.end();
    };

    auto wake = [this, haveUnblockedDirectivesToHandle]() {
        return !m_cancelingQueue.empty() || (!m_handlingQueue.empty() && haveUnblockedDirectivesToHandle()) ||
               m_isShuttingDown;
    };

    std::unique_lock<std::mutex> lock(m_mutex);
    do {
        m_wakeProcessingLoop.wait(lock, wake);

        bool cancelHandled = false;
        bool queuedHandled = false;

        /*
         * Loop to make sure all waiting directives have been
         * handled before exiting thread, in case m_isShuttingDown is true.
         */
        do {
            cancelHandled = processCancelingQueueLocked(lock);
            queuedHandled = handleQueuedDirectivesLocked(lock);
        } while (cancelHandled || queuedHandled);

    } while (!m_isShuttingDown);
}

bool DirectiveProcessor::processCancelingQueueLocked(std::unique_lock<std::mutex>& lock) {
    ACSDK_DEBUG9(LX("processCancelingQueueLocked").d("size", m_cancelingQueue.size()));

    if (m_cancelingQueue.empty()) {
        return false;
    }
    std::deque<std::shared_ptr<avsCommon::avs::AVSDirective>> temp;
    std::swap(m_cancelingQueue, temp);
    lock.unlock();
    for (auto directive : temp) {
        m_directiveRouter->cancelDirective(directive);
    }
    lock.lock();
    return true;
}

bool DirectiveProcessor::handleQueuedDirectivesLocked(std::unique_lock<std::mutex>& lock) {
    if (m_handlingQueue.empty()) {
        return false;
    }
    bool handleDirectiveCalled = false;

    ACSDK_DEBUG9(LX("handleQueuedDirectivesLocked").d("queue size", m_handlingQueue.size()));

    while (!m_handlingQueue.empty()) {
        auto it = getNextUnblockedDirectiveLocked();

        // All directives are blocked - exit loop.
        if (it == m_handlingQueue.end()) {
            ACSDK_DEBUG9(LX("handleQueuedDirectivesLocked").m("all queued directives are blocked"));
            break;
        }
        auto directive = it->first;
        auto policy = it->second;

        setDirectiveBeingHandledLocked(directive, policy);
        m_handlingQueue.erase((it));

        ACSDK_DEBUG9(LX("handleQueuedDirectivesLocked")
                         .d("proceeding with directive", directive->getMessageId())
                         .d("policy", policy));

        handleDirectiveCalled = true;
        lock.unlock();

        auto handleDirectiveSucceeded = m_directiveRouter->handleDirective(directive);

        lock.lock();

        // if handle failed or directive is not blocking
        if (!handleDirectiveSucceeded || !policy.isBlocking()) {
            clearDirectiveBeingHandledLocked(policy);
        }

        if (!handleDirectiveSucceeded) {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("message id", directive->getMessageId()));
            scrubDialogRequestIdLocked(directive->getDialogRequestId());
        }
    }

    return handleDirectiveCalled;
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

    /*
     * If a matching directive is in the midst of a handleDirective() call or a blocking directive which
     * hasn't been completed, reset m_directivesBeinHandle so we won't block processing subsequent directives.
     * This directive is moved to m_cancelingQueue, below.
     */
    auto freed = clearDirectiveBeingHandledLocked([dialogRequestId](const std::shared_ptr<AVSDirective>& directive) {
        return directive->getDialogRequestId() == dialogRequestId;
    });

    if (!freed.empty()) {
        m_cancelingQueue.insert(m_cancelingQueue.end(), freed.begin(), freed.end());
        changed = true;
    }

    // Filter matching directives from m_handlingQueue and put them in m_cancelingQueue.
    std::deque<DirectiveAndPolicy> temp;
    for (const auto& directiveAndPolicy : m_handlingQueue) {
        auto id = (directiveAndPolicy.first)->getDialogRequestId();
        if (!id.empty() && id == dialogRequestId) {
            m_cancelingQueue.push_back(directiveAndPolicy.first);
            changed = true;
        } else {
            temp.push_back(directiveAndPolicy);
        }
    }
    std::swap(temp, m_handlingQueue);

    // If the dialogRequestId to scrub is the current value, reset the current value.
    if (dialogRequestId == m_dialogRequestId) {
        m_dialogRequestId.clear();
    }

    // If there were any changes, wake up the processing loop.
    if (changed) {
        ACSDK_DEBUG9(LX("notifyingProcessingLoop").d("size:", m_cancelingQueue.size()));
        m_wakeProcessingLoop.notify_one();
    }
}

void DirectiveProcessor::queueAllDirectivesForCancellationLocked() {
    ACSDK_DEBUG9(LX("queueAllDirectivesForCancellationLocked"));

    bool changed = false;

    m_dialogRequestId.clear();

    auto freed = clearDirectiveBeingHandledLocked([](const std::shared_ptr<AVSDirective>& directive) { return true; });

    if (!freed.empty()) {
        changed = true;
        m_cancelingQueue.insert(m_cancelingQueue.end(), freed.begin(), freed.end());
    }

    if (!m_handlingQueue.empty()) {
        std::transform(
            m_handlingQueue.begin(),
            m_handlingQueue.end(),
            std::back_inserter(m_cancelingQueue),
            [](DirectiveAndPolicy directiveAndPolicy) { return directiveAndPolicy.first; });

        m_handlingQueue.clear();
        changed = true;
    }

    if (m_directiveBeingPreHandled) {
        m_cancelingQueue.push_back(m_directiveBeingPreHandled);
        m_directiveBeingPreHandled.reset();
    }

    if (changed) {
        ACSDK_DEBUG9(LX("notifyingProcessingLoop"));

        m_wakeProcessingLoop.notify_one();
    }
}

}  // namespace adsl
}  // namespace alexaClientSDK
