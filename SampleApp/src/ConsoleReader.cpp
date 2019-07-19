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

#include <iostream>

#include "SampleApp/ConsolePrinter.h"
#include "SampleApp/ConsoleReader.h"

namespace alexaClientSDK {
namespace sampleApp {

/// Time to wait for if there's a pending read request from the user.
static const std::chrono::milliseconds READ_REQUEST_PENDING_TIMEOUT{100};

ConsoleReader::ConsoleReader() : m_state{IDLE}, m_shutDown{false}, m_lastCharRead{0} {
    m_thread = std::thread(&ConsoleReader::workerLoop, this);
}

ConsoleReader::~ConsoleReader() {
    m_shutDown = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool ConsoleReader::read(const std::chrono::milliseconds timeout, char* data) {
    if (!data) {
        ConsolePrinter::simplePrint("ConsoleReader read failed due to null data pointer.");
        return false;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (State::IDLE == m_state) {
        m_state = State::PENDING_REQUEST;
        m_waitOnEvent.notify_one();
    }

    if (m_isDataAvailable.wait_for(lock, timeout, [this] { return State::DATA_READY == m_state; })) {
        m_state = State::IDLE;
        *data = m_lastCharRead;
        return true;
    } else {
        return false;
    }
}

void ConsoleReader::workerLoop() {
    while (!m_shutDown) {
        char tempChar;
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_waitOnEvent.wait_for(lock, READ_REQUEST_PENDING_TIMEOUT, [this] {
                return m_shutDown || State::PENDING_REQUEST == m_state;
            })) {
            if (m_shutDown) {
                break;
            }
            m_state = State::READING;
            lock.unlock();

            std::cin >> tempChar;

            lock.lock();
            m_lastCharRead = tempChar;
            m_state = State::DATA_READY;
            m_isDataAvailable.notify_one();
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
