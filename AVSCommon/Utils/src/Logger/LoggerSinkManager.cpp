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

#include "AVSCommon/Utils/Logger/LoggerSinkManager.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

LoggerSinkManager& LoggerSinkManager::instance() {
    static LoggerSinkManager singleLoggerSinkManager;
    return singleLoggerSinkManager;
}

void LoggerSinkManager::addSinkObserver(SinkObserverInterface* observer) {
    if (!observer) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_sinkObserverMutex);
        m_sinkObservers.push_back(observer);
    }

    // notify this observer of the current sink right away
    observer->onSinkChanged(m_sink);
}

void LoggerSinkManager::removeSinkObserver(SinkObserverInterface* observer) {
    if (!observer) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_sinkObserverMutex);
    m_sinkObservers.erase(std::remove(m_sinkObservers.begin(), m_sinkObservers.end(), observer), m_sinkObservers.end());
}

void LoggerSinkManager::initialize(const std::shared_ptr<Logger>& sink) {
    if (m_sink == sink) {
        // don't do anything if the sink is the same
        return;
    }

    // copy the vector first with the lock
    std::vector<SinkObserverInterface*> observersCopy;
    {
        std::lock_guard<std::mutex> lock(m_sinkObserverMutex);
        observersCopy = m_sinkObservers;
    }

    m_sink = sink;

    // call the callbacks
    for (auto observer : observersCopy) {
        observer->onSinkChanged(m_sink);
    }
}

LoggerSinkManager::LoggerSinkManager() : m_sink(ACSDK_GET_SINK_LOGGER()) {
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
