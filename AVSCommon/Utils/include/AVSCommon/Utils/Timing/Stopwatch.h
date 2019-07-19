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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_STOPWATCH_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_STOPWATCH_H_

#include <chrono>
#include <mutex>
#include <thread>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * Class to provide simple stopwatch functionality.
 */
class Stopwatch {
public:
    /**
     * Constructor.  Stopwatch instances are created ready for a call to @c start.
     */
    Stopwatch();

    /**
     * Start marking time.  Only valid if called when the stopwatch is new or reset().
     *
     * @return Whether the operation succeeded.
     */
    bool start();

    /**
     * Pause marking time.  Only valid if called after @c start() or @c resume().
     *
     * @return Whether the operation succeeded.
     */
    bool pause();

    /**
     * Resume marking time.  Only valid if called after @c pause().
     *
     * @return Whether the operation succeeded.
     */
    bool resume();

    /**
     * Stop marking time.  Valid after all other calls.
     */
    void stop();

    /**
     * Reset elapsed time, prepare for @c start().  Valid after all other calls.
     */
    void reset();

    /**
     * Get the total time elapsed in the @c start() or @c resume() state.
     *
     * @return The total time elapsed in the @c start() or @c resume() state.
     */
    std::chrono::milliseconds getElapsed();

private:
    /**
     * Emum specifying the current state of a @c Stopwatch.
     */
    enum class State {
        /// Initial / reset state. Elapsed time reset to zero.  Ready to start.
        RESET,
        /// The @c started()ed or @c resume()ed state. Time is being marked.
        RUNNING,
        /// The @c pause()d state. Time is not being marked.
        PAUSED,
        /// The @c stop()ed state. Time is not being marked.  Elapsed reflects total time spent
        /// @c start()ed and @c resume()d
        STOPPED
    };

    /// Serializes access to all members.
    std::mutex m_mutex;

    /// The current state of the Stopwatch.
    State m_state;

    /// The time @c start() was called while @c RESET.
    std::chrono::steady_clock::time_point m_startTime;

    /// The last time @c pause() was called while @c RUNNING.
    std::chrono::steady_clock::time_point m_pauseTime;

    /// The last time @c stop() was called while @c RUNNING or @c PAUSED.
    std::chrono::steady_clock::time_point m_stopTime;

    /// The total time spent @c PAUSED.
    std::chrono::milliseconds m_totalTimePaused;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_STOPWATCH_H_
