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

#include "Integration/TestSpeechSynthesizerObserver.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface;

TestSpeechSynthesizerObserver::TestSpeechSynthesizerObserver() :
        m_state(SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED) {
}

void TestSpeechSynthesizerObserver::onStateChanged(SpeechSynthesizerObserverInterface::SpeechSynthesizerState state) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_state = state;
    m_queue.push_back(state);
    m_wakeTrigger.notify_all();
}

bool TestSpeechSynthesizerObserver::checkState(
    const SpeechSynthesizerObserverInterface::SpeechSynthesizerState expectedState,
    const std::chrono::seconds duration) {
    // Pull the front of the state queue
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState hold = waitForNext(duration);
    return hold == expectedState;
}

SpeechSynthesizerObserverInterface::SpeechSynthesizerState TestSpeechSynthesizerObserver::waitForNext(
    const std::chrono::seconds duration) {
    SpeechSynthesizerObserverInterface::SpeechSynthesizerState ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        return m_state;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

SpeechSynthesizerObserverInterface::SpeechSynthesizerState TestSpeechSynthesizerObserver::getCurrentState() {
    return m_state;
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
