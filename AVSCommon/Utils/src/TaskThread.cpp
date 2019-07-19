/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Threading/TaskThread.h"

/// String to identify log entries originating from this file.
static const std::string TAG("TaskThread");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

using namespace logger;

TaskThread::TaskThread() : m_alreadyStarting{false}, m_moniker{ThreadMoniker::generateMoniker()} {
}

TaskThread::~TaskThread() {
    m_stop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool TaskThread::start(std::function<bool()> jobRunner) {
    if (!jobRunner) {
        ACSDK_ERROR(LX("startFailed").d("reason", "invalidFunction"));
        return false;
    }

    bool notRunning = false;
    if (!m_alreadyStarting.compare_exchange_strong(notRunning, true)) {
        ACSDK_ERROR(LX("startFailed").d("reason", "tooManyThreads"));
        return false;
    }

    m_oldThread = std::move(m_thread);
    m_thread = std::thread{std::bind(&TaskThread::run, this, std::move(jobRunner))};
    return true;
}

void TaskThread::run(std::function<bool()> jobRunner) {
    if (m_oldThread.joinable()) {
        m_stop = true;
        m_oldThread.join();
    }

    // Reset stop flag and already starting flag.
    m_stop = false;
    m_alreadyStarting = false;
    ThreadMoniker::setThisThreadMoniker(m_moniker);

    while (!m_stop && jobRunner())
        ;
}

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
