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

#include "acsdk/VisualTimeoutManager/private/VisualTimeoutManager.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace visualTimeoutManager {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace presentationOrchestratorInterfaces;
using namespace std::chrono;

/// String to identify log entries originating from this file.
#define TAG "VisualTimeoutManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

std::shared_ptr<VisualTimeoutManager> VisualTimeoutManager::create(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>
        timerDelegateFactory) {
    auto visualTimeoutManager = std::shared_ptr<VisualTimeoutManager>(new VisualTimeoutManager(timerDelegateFactory));

    return visualTimeoutManager;
}

VisualTimeoutManager::VisualTimeoutManager(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>
        timerDelegateFactory) :
        RequiresShutdown{TAG},
        m_timer{timerDelegateFactory},
        m_dialogUxState{DialogUXState::IDLE},
        m_timeoutIdCounter{0} {
    m_executor = std::make_shared<threading::Executor>();
}

VisualTimeoutManagerInterface::VisualTimeoutId VisualTimeoutManager::requestTimeout(
    milliseconds delay,
    VisualTimeoutManagerInterface::VisualTimeoutCallback timeoutCallback) {
    ACSDK_DEBUG5(LX(__func__).d("timeoutMs", delay.count()));
    return m_executor
        ->submit([this, delay, timeoutCallback] {
            // stop previous active timer
            if (m_timer.isActive()) {
                ACSDK_DEBUG9(LX("requestTimeout").d("reason", "stopping previous timer"));
                m_timer.stop();
            }
            // reset any sources tracking activity
            m_activeSources.clear();

            m_currentTimeoutAttributes = VisualTimeoutAttributes(delay, timeoutCallback, m_timeoutIdCounter++);

            executeStartTimer();
            return m_currentTimeoutAttributes.timeoutId;
        })
        .get();
}

bool VisualTimeoutManager::stopTimeout(VisualTimeoutManagerInterface::VisualTimeoutId timeoutId) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor->submit([this, timeoutId]() { return executeStopTimer(timeoutId); }).get();
}

void VisualTimeoutManager::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, newState]() { executeOnDialogUXStateChanged(newState); });
}

void VisualTimeoutManager::onGUIActivityEventReceived(
    const std::string& source,
    const GUIActivityEvent& activityEvent) {
    ACSDK_DEBUG5(LX(__func__));
    if (source.empty()) {
        ACSDK_ERROR(LX("onActivityEventReceivedFailed").d("reason", "event source is empty"));
        return;
    }

    m_executor->submit([this, source, activityEvent]() { executeOnGUIActivityEventReceived(source, activityEvent); });
}

void VisualTimeoutManager::doShutdown() {
    m_timer.stop();
    m_executor->shutdown();
}

void VisualTimeoutManager::setExecutor(const std::shared_ptr<threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).d("reason", "should only be called in tests"));
    m_executor = executor;
}

void VisualTimeoutManager::executeStartTimer() {
    ACSDK_DEBUG5(LX("executeStartTimer").d("timeoutMs", m_currentTimeoutAttributes.delay.count()));

    if (DialogUXStateObserverInterface::DialogUXState::IDLE == m_dialogUxState && m_activeSources.empty()) {
        m_timer.start(m_currentTimeoutAttributes.delay, [this] { m_executor->submit([this] { executeCallback(); }); });
        executeSetState(VisualTimeoutState::ACTIVE);
    } else {
        executeSetState(VisualTimeoutState::SUSPENDED);
    }
}

bool VisualTimeoutManager::executeStopTimer(VisualTimeoutManagerInterface::VisualTimeoutId timeoutId) {
    ACSDK_DEBUG5(LX("executeStopTimer"));
    if (timeoutId != m_currentTimeoutAttributes.timeoutId) {
        ACSDK_DEBUG9(
            LX("executeStopTimer").d("timeoutId", timeoutId).d("reason", "timeout Id does not match active timer"));
        return false;
    }

    if (m_timer.isActive()) {
        m_timer.stop();
    }

    executeSetState(VisualTimeoutState::STOPPED);
    return true;
}

void VisualTimeoutManager::executeOnDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG5(LX("executeOnDialogUXStateChanged"));
    m_dialogUxState = newState;

    if (DialogUXStateObserverInterface::DialogUXState::IDLE == newState && m_activeSources.empty()) {
        if (m_currentTimeoutAttributes.state == VisualTimeoutState::SUSPENDED) {
            ACSDK_DEBUG9(
                LX(__func__).d("timeoutId", m_currentTimeoutAttributes.timeoutId).d("reason", "restarting timer"));
            executeStartTimer();
        }
        return;
    }

    if (m_timer.isActive()) {
        ACSDK_DEBUG9(LX(__func__).d("timeoutId", m_currentTimeoutAttributes.timeoutId).d("reason", "suspending timer"));
        m_timer.stop();
        executeSetState(VisualTimeoutState::SUSPENDED);
    }
}

void VisualTimeoutManager::executeCallback() {
    ACSDK_DEBUG5(LX("executeCallback"));

    // ensure that the timeout is still active
    if (m_currentTimeoutAttributes.state != VisualTimeoutState::ACTIVE) {
        ACSDK_DEBUG9(LX("executeCallback")
                         .d("currentState", m_currentTimeoutAttributes.state)
                         .d("reason", "callback not executed as timeout is no longer active"));
        return;
    }

    m_currentTimeoutAttributes.timeoutCallback();
    executeSetState(VisualTimeoutState::FINISHED);
}

void VisualTimeoutManager::executeOnGUIActivityEventReceived(
    const std::string& source,
    const GUIActivityEvent& activityEvent) {
    ACSDK_DEBUG5(LX(__func__).d("source", source).d("event", activityEvent));

    switch (activityEvent) {
        case GUIActivityEvent::ACTIVATED:
            if (m_timer.isActive()) {
                ACSDK_DEBUG9(
                    LX(__func__).d("timeoutId", m_currentTimeoutAttributes.timeoutId).d("reason", "suspending timer"));
                m_timer.stop();
                executeSetState(VisualTimeoutState::SUSPENDED);
            }
            m_activeSources.insert(source);
            break;

        case GUIActivityEvent::DEACTIVATED:
            m_activeSources.erase(source);
            // fall through; no break
        case GUIActivityEvent::INTERRUPT:
            if (DialogUXState::IDLE == m_dialogUxState && m_activeSources.empty()) {
                if (m_currentTimeoutAttributes.state == VisualTimeoutState::SUSPENDED) {
                    ACSDK_DEBUG9(LX(__func__)
                                     .d("timeoutId", m_currentTimeoutAttributes.timeoutId)
                                     .d("reason", "restarting timer"));
                    executeStartTimer();
                }
            }
            break;

        case GUIActivityEvent::UNKNOWN:
            ACSDK_WARN(LX(__func__).d("event", activityEvent));
            break;
    }
}

void VisualTimeoutManager::executeSetState(const VisualTimeoutState& newState) {
    ACSDK_DEBUG9(LX("executeSetState").d("newState", newState));
    m_currentTimeoutAttributes.state = newState;
}

std::ostream& operator<<(std::ostream& stream, const VisualTimeoutManager::VisualTimeoutState& state) {
    return stream << VisualTimeoutManager::visualTimeoutStateToString(state);
}

}  // namespace visualTimeoutManager
}  // namespace alexaClientSDK