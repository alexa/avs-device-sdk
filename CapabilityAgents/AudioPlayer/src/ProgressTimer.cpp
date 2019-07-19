/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include "AudioPlayer/ProgressTimer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

using namespace avsCommon::utils::timing;

const std::chrono::milliseconds ProgressTimer::NO_DELAY{std::chrono::milliseconds::max()};
const std::chrono::milliseconds ProgressTimer::NO_INTERVAL{std::chrono::milliseconds::max()};

/// String to identify log entries originating from this file.
static const std::string TAG("ProgressTimer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

ProgressTimer::ProgressTimer() :
        m_state{State::IDLE},
        m_delay{NO_DELAY},
        m_interval{NO_INTERVAL},
        m_target{std::chrono::milliseconds::zero()},
        m_gotProgress{false},
        m_progress{std::chrono::milliseconds::zero()} {
}

/**
 * Write a @c ProgressTimer::State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
std::ostream& operator<<(std::ostream& stream, ProgressTimer::State state) {
    switch (state) {
        case ProgressTimer::State::IDLE:
            return stream << "IDLE";
        case ProgressTimer::State::INITIALIZED:
            return stream << "INITIALIZED";
        case ProgressTimer::State::RUNNING:
            return stream << "RUNNING";
        case ProgressTimer::State::PAUSED:
            return stream << "PAUSED";
        case ProgressTimer::State::STOPPING:
            return stream << "STOPPING";
    }
    return stream << "Unknown state: " << static_cast<int>(state);
}

ProgressTimer::~ProgressTimer() {
    stop();
}

void ProgressTimer::init(
    const std::shared_ptr<ProgressTimer::ContextInterface>& context,
    std::chrono::milliseconds delay,
    std::chrono::milliseconds interval,
    std::chrono::milliseconds offset) {
    ACSDK_DEBUG5(LX(__func__).d("delay", delay.count()).d("interval", interval.count()).d("offset", offset.count()));

    if (!context) {
        ACSDK_ERROR(LX("initFailed").d("reason", "nullContext"));
        return;
    }

    if (std::chrono::milliseconds::zero() == interval) {
        ACSDK_ERROR(LX("initFailed").d("reason", "intervalWasZero"));
        return;
    }

    std::lock_guard<std::mutex> callLock(m_callMutex);

    if (!setState(State::INITIALIZED)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "setStateFailed"));
        return;
    }

    m_context = context;
    m_interval = interval;
    m_delay = delay >= offset ? delay : NO_DELAY;
    m_offset = offset;
    m_progress = offset;
}

void ProgressTimer::start() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> callLock(m_callMutex);

    if (!setState(State::RUNNING)) {
        ACSDK_ERROR(LX("startFailed").d("reason", "setStateFailed"));
        return;
    }

    if (m_delay != NO_DELAY) {
        if (m_interval != NO_INTERVAL) {
            m_target = std::min(m_delay, m_interval * ((m_offset / m_interval) + 1));
        } else {
            m_target = m_delay;
        }
    } else {
        if (m_interval != NO_INTERVAL) {
            m_target = m_interval * ((m_offset / m_interval) + 1);
        } else {
            ACSDK_DEBUG5(LX("startNotStartingThread").d("reason", "noTarget"));
            setState(State::IDLE);
            m_context.reset();
            return;
        }
    }

    startThread();
}

void ProgressTimer::pause() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> callLock(m_callMutex);

    if (!setState(State::PAUSED)) {
        ACSDK_ERROR(LX("pauseFailed").d("reason", "setStateFailed"));
        return;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ProgressTimer::resume() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> callLock(m_callMutex);

    if (!setState(State::RUNNING)) {
        ACSDK_ERROR(LX("resumeFailed").d("reason", "setStateFailed"));
        return;
    }

    startThread();
}

void ProgressTimer::stop() {
    ACSDK_DEBUG5(LX(__func__));

    // If we are already stopped, there is nothing to do.
    {
        std::lock_guard<std::mutex> stateLock(m_stateMutex);
        if (State::IDLE == m_state) {
            return;
        }
    }

    std::lock_guard<std::mutex> callLock(m_callMutex);

    if (!setState(State::STOPPING)) {
        ACSDK_ERROR(LX("stopFailed").d("reason", "setStateFailed"));
        return;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (!setState(State::IDLE)) {
        ACSDK_ERROR(LX("stopFailed").d("reason", "setStateFailed"));
        return;
    }

    m_context.reset();
    m_delay = NO_DELAY;
    m_interval = NO_INTERVAL;
    m_target = std::chrono::milliseconds::zero();
}

void ProgressTimer::onProgress(std::chrono::milliseconds progress) {
    ACSDK_DEBUG5(LX(__func__).d("progress", progress.count()));

    std::lock_guard<std::mutex> stateLock(m_stateMutex);

    m_progress = progress;
    m_gotProgress = true;
    m_wake.notify_all();
}

bool ProgressTimer::setState(State newState) {
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    bool allowed = false;
    switch (newState) {
        case State::IDLE:
            allowed = State::RUNNING == m_state || State::STOPPING == m_state;
            break;
        case State::INITIALIZED:
            allowed = State::IDLE == m_state;
            break;
        case State::RUNNING:
            allowed = State::INITIALIZED == m_state || State::PAUSED == m_state;
            break;
        case State::PAUSED:
            allowed = State::RUNNING == m_state;
            break;
        case State::STOPPING:
            allowed = true;
            break;
    }

    if (allowed) {
        ACSDK_DEBUG9(LX(__func__).d("state", m_state).d("newState", newState));
        m_state = newState;
        m_wake.notify_one();
    } else {
        ACSDK_ERROR(LX("setStateFailed").d("reason", "notAllowed").d("state", m_state).d("newState", newState));
    }

    return allowed;
}

void ProgressTimer::startThread() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_thread = std::thread(&ProgressTimer::mainLoop, this);
}

void ProgressTimer::mainLoop() {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> stateLock(m_stateMutex);

    if (NO_DELAY == m_delay && NO_INTERVAL == m_interval) {
        ACSDK_DEBUG5(LX("mainLoopExiting").d("reason", "noDelayOrInterval"));
        return;
    }

    while (State::RUNNING == m_state) {
        stateLock.unlock();
        m_gotProgress = false;
        m_context->requestProgress();
        stateLock.lock();

        m_wake.wait(stateLock, [this] { return m_state != State::RUNNING || m_gotProgress; });

        if (m_state != State::RUNNING) {
            break;
        }

        if (m_progress >= m_target) {
            if (m_target == m_delay) {
                m_context->onProgressReportDelayElapsed();
                // If delay and interval coincide, send both notifications.
                if (m_interval != NO_INTERVAL && (m_target.count() % m_interval.count()) == 0) {
                    m_context->onProgressReportIntervalElapsed();
                }
            } else {
                m_context->onProgressReportIntervalElapsed();
            }
            if (!updateTargetLocked()) {
                ACSDK_DEBUG5(LX("mainLoopExiting").d("reason", "noTarget"));
                return;
            }
        } else {
            std::chrono::milliseconds timeout = m_target - m_progress;
            m_wake.wait_for(stateLock, timeout, [this] { return m_state != State::RUNNING; });
        }
    }

    ACSDK_DEBUG5(LX("mainLoopExiting").d("reason", "isStopping"));
}

bool ProgressTimer::updateTargetLocked() {
    ACSDK_DEBUG9(LX(__func__).d("target", m_target.count()).d("progress", m_progress.count()));

    // This method checks if the current state of progress has passed the current target and
    // if it has, it updates target to the offset at which the next progress report should be sent.
    // It then returns true if there is a target to proceed to, or false if there are no more
    // progress reports to send.  The rules for interpreting the delay and interval values are
    // explained in progressReportDelayElapsed event and progressReportIntervalElapsed event
    // sections of the AudioPlayer interface documentation:
    //     https://developer.amazon.com/docs/alexa-voice-service/audioplayer.html

    // Haven't reached the target yet, so no need to update it.
    if (m_progress < m_target) {
        return true;
    }

    // No reporting after an initial delay.
    if (NO_DELAY == m_delay) {
        // If no periodic reports, either, there will be no progress reports, and so, no target.
        if (NO_INTERVAL == m_interval) {
            ACSDK_DEBUG9(LX("noTarget"));
            return false;
        }

        // To handle reporting progress periodically, simply step the target by the interval.
        m_target += m_interval;
        ACSDK_DEBUG9(LX("newTarget").d("target", m_target.count()));
        return true;
    }

    // Handle reporting progress after an initial delay, and without reporting periodic progress.
    if (NO_INTERVAL == m_interval) {
        // If progress has already reached the initial delay and there is no interval, there is
        // no more progress to report and mainLoop() will exit.  Reset m_delay before returning
        // so that a pesky call to resume() won't trigger more progress reports.
        if (m_target == m_delay) {
            m_delay = NO_DELAY;
            return false;
        }

        // ...otherwise the target is the delay.
        m_target = m_delay;
        ACSDK_DEBUG9(LX("newTarget").d("target", m_target.count()));
        return true;
    }

    // Handle reporting progress periodically, as well as after an initial delay.
    if (m_target < m_delay) {
        // Target is still before the start delay. To find the next target, add the interval, if
        // that passes the start delay, use the start delay instead.
        m_target += m_interval;
        if (m_target > m_delay) {
            m_target = m_delay;
        }
    } else if (m_target == m_delay) {
        // Target is the start delay.  Find the next regular interval after the start delay.
        auto count = (m_delay.count() / m_interval.count()) + 1;
        m_target = m_interval * count;
    } else {
        // Target already past the start delay.  Just keep incrementing it.
        m_target += m_interval;
    }

    ACSDK_DEBUG9(LX("newTarget").d("target", m_target.count()));
    return true;
}

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
