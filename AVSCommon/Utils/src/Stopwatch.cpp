/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Timing/Stopwatch.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/// String to identify log entries originating from this file.
static const std::string TAG("Stopwatch");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Helper method to get the elapsed time in milliseconds from two @c time_point values.
 *
 * @param later The end time for the calculation .
 * @param earlier The start time for the calculation.
 * @return The elapsed time in milliseconds from two @c time_point values.
 */
static std::chrono::milliseconds elapsed(
    std::chrono::steady_clock::time_point later,
    std::chrono::steady_clock::time_point earlier) {
    // Not expected, but just in case...
    if (earlier >= later) {
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(later - earlier);
}

Stopwatch::Stopwatch() {
    reset();
}

bool Stopwatch::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != State::RESET) {
        ACSDK_ERROR(LX("startFailed").d("reason", "stateNotRESET"));
        return false;
    }
    m_startTime = std::chrono::steady_clock::now();
    m_state = State::RUNNING;
    return true;
}

bool Stopwatch::pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != State::RUNNING) {
        ACSDK_ERROR(LX("pauseFailed").d("reason", "stateNotRUNNING"));
        return false;
    }
    m_pauseTime = std::chrono::steady_clock::now();
    m_state = State::PAUSED;
    return true;
}

bool Stopwatch::resume() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != State::PAUSED) {
        ACSDK_ERROR(LX("resumeFailed").d("reason", "stateNotPAUSED"));
        return false;
    }
    m_totalTimePaused += elapsed(std::chrono::steady_clock::now(), m_pauseTime);
    m_state = State::RUNNING;
    return true;
}

void Stopwatch::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != State::RESET && m_state != State::STOPPED) {
        m_stopTime = std::chrono::steady_clock::now();
    }
    m_state = State::STOPPED;
}

void Stopwatch::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = State::RESET;
    m_startTime = std::chrono::steady_clock::time_point();
    m_pauseTime = std::chrono::steady_clock::time_point();
    m_stopTime = std::chrono::steady_clock::time_point();
    m_totalTimePaused = std::chrono::milliseconds::zero();
}

std::chrono::milliseconds Stopwatch::getElapsed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (m_state) {
        case State::RESET:
            return std::chrono::milliseconds::zero();
        case State::RUNNING:
            return elapsed(std::chrono::steady_clock::now(), m_startTime) - m_totalTimePaused;
        case State::PAUSED:
            return elapsed(m_pauseTime, m_startTime) - m_totalTimePaused;
        case State::STOPPED:
            return elapsed(m_stopTime, m_startTime) - m_totalTimePaused;
    }
    // We should never get here.
    return std::chrono::milliseconds::zero();
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
