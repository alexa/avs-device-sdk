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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGERSINKMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGERSINKMANAGER_H_

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
     * Initialize the sink logger managed by the manager.
     * This function can be called only before any other threads in the process have been created by the
     * program.
     *
     * @param sink The new @c Logger to forward logs to.
     *
     * @note If this function is not called, the default sink logger
     * will be the one returned by get<ACSDK_LOG_SINK>Logger().
     */
    void initialize(const std::shared_ptr<Logger>& sink);

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
    std::shared_ptr<Logger> m_sink;
};

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGERSINKMANAGER_H_
