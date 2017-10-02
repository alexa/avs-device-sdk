/*
 * LoggerSinkManager.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGER_LOGGER_SINK_MANAGER_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGER_LOGGER_SINK_MANAGER_H_

#include <atomic>
#include <mutex>
#include <vector>

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * A manager to manage the sink logger and notify SinkObservers of any changes.
 */
class LoggerSinkManager {
public:
    /**
     * Return the one and only @c LoggerSinkManager instance.
     *
     * @return The one and only @c LoggerSinkManager instance.
     */
    static LoggerSinkManager& instance();

    /**
     * Add a SinkObserver to the manager.
     *
     * @param observer The @c SinkObserverInterface be be added.
     */
    void addSinkObserver(SinkObserverInterface* observer);

    /**
     * Remove a SinkObserver from the manager.
     *
     * @param observer The @c SinkObserverInterface to be removed.
     */
    void removeSinkObserver(SinkObserverInterface* observer);

    /**
     * Change the sink logger managed by the manager.
     *
     * @param sink The new @c Logger to forward logs to.
     *
     * @note It is up to the application to serialize calls to changeSinkLogger.
     */
    void changeSinkLogger(Logger& sink);

private:
    /**
     * Constructor.
     */
    LoggerSinkManager();

    /// This mutex guards access to m_sinkObservers.
    std::mutex m_sinkObserverMutex;

    /// Vector of SinkObservers to be managed.
    std::vector<SinkObserverInterface*> m_sinkObservers;

    /// The @c Logger to forward logs to.
    std::atomic<Logger*> m_sink;
};

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGER_LOGGER_SINK_MANAGER_H_
