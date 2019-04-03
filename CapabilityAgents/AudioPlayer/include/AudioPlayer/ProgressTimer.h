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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PROGRESSTIMER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PROGRESSTIMER_H_

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/**
 * Provides callbacks when ProgressReportDelayElapsed and ProgressReportIntervalElapsed events
 * should be sent to AVS.
 */
class ProgressTimer {
public:
    /**
     * Interface to the context within which ProgressTimer operates.  This interface
     * provides a way to get the current offset of playback, and methods to trigger sending
     * progress callbacks.
     */
    class ContextInterface {
    public:
        /**
         * Request a (potentially asynchronous) callback to onProgress() with the current progress.
         */
        virtual void requestProgress() = 0;

        /**
         * Notification that it is time to send a ProgressReportDelayElapsed event.
         */
        virtual void onProgressReportDelayElapsed() = 0;

        /**
         * Notification that it is time to send a ProgressReportIntervalElapsed event.
         */
        virtual void onProgressReportIntervalElapsed() = 0;

        /**
         * Destructor.
         */
        virtual ~ContextInterface() = default;
    };

    /// Delay value for no ProgressReportDelayElapsed notifications.
    static const std::chrono::milliseconds NO_DELAY;

    /// Interval value for no ProgressReportIntervalElapsed notifications.
    static const std::chrono::milliseconds NO_INTERVAL;

    /**
     * Constructor.
     */
    ProgressTimer();

    /**
     * Destructor.
     */
    virtual ~ProgressTimer();

    /**
     * Initialize for sending notifications that it is time to send a progress report.
     * @c init() must be called before @c start() (without an intervening @c stop()) for @c start()
     * to deliver progress report callbacks.
     *
     * @param context The context within which to operate.
     * @param delay The offset (in milliseconds from the start of the track) at which to send the
     * @c ProgressReportDelayElapsed event. If delay is @c NO_DELAY, no @c ProgressReportDelayElapsed
     * notifications will be sent.
     * @param interval The interval (in milliseconds from the start of the track) at which to send
     * @c ProgressReportIntervalElapsed events. If interval is @c NO_INTERVAL, no
     * @c ProgressReportIntervalElapsed notifications will be sent.
     * @param offset The offset (in milliseconds from the start of the track) at which playback of
     * the track will start.
     */
    void init(
        const std::shared_ptr<ContextInterface>& context,
        std::chrono::milliseconds delay,
        std::chrono::milliseconds interval,
        std::chrono::milliseconds offset = std::chrono::milliseconds::zero());

    /**
     * Start sending notifications when it is time to send progress reports.
     * @c init() must be called before @c start() (without an intervening @c stop()) for @c start()
     * to deliver progress report callbacks.
     */
    void start();

    /**
     * Pause sending notifications when it is time to send progress reports.
     * @c pause() should be called after @c start() or @c resume() (without an intervening @c stop())
     * or it will do nothing.
     */
    void pause();

    /**
     * Resume sending notifications when it is time to send progress reports.
     * @c resume() should be called after @c pause() (without an intervening @c stop()) otherwise
     * it will do nothing.
     */
    void resume();

    /**
     * Stop sending notifications when it is time to send progress reports.
     * @c stop() can be called after @c init(), @c start(), @c pause(), and @c resume() to reset
     * a @c ProgressTimer to it's pre @c init() state.
     */
    void stop();

    /**
     * Notification of the current progress.
     *
     * @param progress The offset (in milliseconds from the start of the track) of playback.
     */
    void onProgress(std::chrono::milliseconds progress);

private:
    /**
     * Enum representing the state of a ProgressTimer instance.
     */
    enum class State {
        /// Fully stopped and de-initialized.
        IDLE,
        /// Initialized (ready to start when audio playback starts).
        INITIALIZED,
        /// Periodically sending notifications.
        RUNNING,
        /// Sending notifications has been paused, but the ProgressTimer is ready to resume sending them.
        PAUSED,
        /// The ProgressTimer is in the process of stopping.
        STOPPING,
    };

    /// Friend declaration to allow streaming @c State values.
    friend std::ostream& operator<<(std::ostream& stream, ProgressTimer::State state);

    /**
     * Set the current state.  Also notifies @c m_wake when the state changes.
     *
     * @param newState The state to transition to.
     * @return Whether or not the transition was allowed.
     */
    bool setState(State newState);

    /**
     * Start the thread that runs @c main_loop().
     */
    void startThread();

    /**
     * Thread function that sends notifications when it is time to send progress reports.
     */
    void mainLoop();

    /**
     * Step the target offset at which the next notification should be sent.
     * @c m_stateMutex must be held when this method is called.
     *
     * @return Whether or not there is a target offset to wait for.
     */
    bool updateTargetLocked();

    /// Mutex serializing calls to public methods.
    std::mutex m_callMutex;

    /// Mutex serializing access to @c m_state, @c m_progress and @c m_gotProgress.
    std::mutex m_stateMutex;

    /// The current state of the ProgressTimer.
    State m_state;

    /// The context in which the progress timer operates.
    std::shared_ptr<ContextInterface> m_context;

    /// The offset in to the audio stream at which to send the ProgressReportDelayElapsed event.
    std::chrono::milliseconds m_delay;

    /// The interval between offsets at which to send ProgressReportIntervalElapsed events.
    std::chrono::milliseconds m_interval;

    /// The offset in to the audio stream at which playback will begins
    std::chrono::milliseconds m_offset;

    /// The next offset at which to send a notification.
    std::chrono::milliseconds m_target;

    /// Has progress been reported since last requested.
    bool m_gotProgress;

    /// The last reported progress value.
    std::chrono::milliseconds m_progress;

    /// Condition variable used to wake @c mainLoop() when there is a state change.
    std::condition_variable m_wake;

    /// The thread upon which @c mainLoop() runs.
    std::thread m_thread;
};

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_PROGRESSTIMER_H_
