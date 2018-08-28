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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEREADER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEREADER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * ConsoleReader provides an alternative to std::cin where the users can specify the timeout for an input from the
 * console.
 */
class ConsoleReader {
public:
    /// Constructor.
    ConsoleReader();

    /// Destructor.
    ~ConsoleReader();

    /**
     * Reads an input from the console.  This is a blocking call until an input is read or until the timeout has
     * occurred.
     *
     * @param timeout Timeout to wait for the input.
     * @param[out] data A pointer to a single character.  It will be set to the value of the last character which was
     * read from the console.
     * @return Returns @c true if a character is read from the console.  @c false if timeout has occurred.
     */
    bool read(const std::chrono::milliseconds timeout, char* data);

private:
    /// A worker loop for the @c ConsoleReader.
    void workerLoop();

    /// States for the @c ConsoleReader.
    enum State {
        /// In idle state that is not reading any input from console.
        IDLE,

        /// The user has requested a read and is pending the worker thread to read from console.
        PENDING_REQUEST,

        /// The worker thread is reading from the console.
        READING,

        /// Data is ready.
        DATA_READY
    };

    /// Current state of the @c ConsoleReader.
    State m_state;

    /// This mutex is used to protect @c m_state, @c m_lastCharRead, and the conditional variables @c m_isDataAvailable
    /// and @c m_waitOnEvent.
    std::mutex m_mutex;

    /// A flag to indicate if @c ConsoleReader is being shutdown.
    std::atomic_bool m_shutDown;

    /// A conditional variable to wait and notify that data is available.
    std::condition_variable m_isDataAvailable;

    /// A conditional variable to wait and notify if a user requests a read from console.
    std::condition_variable m_waitOnEvent;

    /// This is used to store the last character read from console.
    char m_lastCharRead;

    /// A thread to where the @c workerLoop() is run.
    std::thread m_thread;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEREADER_H_
