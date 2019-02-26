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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKTHREAD_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKTHREAD_H_

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * A TaskThread executes in sequence until no more tasks exists.
 *
 * @note It's the caller responsibility to restart the @c TaskThread if jobRunner returns false.
 */
class TaskThread {
public:
    /**
     * Constructs a TaskThread to read from the given TaskQueue. This does not start the thread.
     *
     * @params taskQueue A TaskQueue to take tasks from to execute.
     */
    TaskThread();

    /**
     * Destructs the TaskThread.
     */
    ~TaskThread();

    /**
     * Start executing tasks from the given job runner. The task thread will keep running until @c jobRunner
     * returns @c false or @c start gets called again.
     *
     * @param jobRunner Function that should execute jobs. The function should return @c true if there's more tasks
     * to be executed.
     * @return @c true if it succeeds to start the new jobRunner thread; @c false if it fails.
     */
    bool start(std::function<bool()> jobRunner);

private:
    /**
     * Run the @c jobRunner until it returns @c false or @c m_stop is set to true.
     *
     * @param jobRunner Function that should execute the next job. The function should return @c true if a new job
     * still exists.
     */
    void run(std::function<bool()> jobRunner);

    /// The thread to run tasks on.
    std::thread m_thread;

    /// Old thread that will be terminated after start.
    std::thread m_oldThread;

    /// Flag used by the new thread to ensure that the old thread will exit once the current job ends.
    std::atomic_bool m_stop;

    /// Flag used to indicate that there is a new job starting.
    std::atomic_bool m_alreadyStarting;

    /// The task thread moniker.
    std::string m_moniker;
};

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_TASKTHREAD_H_
