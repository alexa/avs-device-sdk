/*
 * TestSpeechSynthesizerObserver.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_TEST_SPEECH_SYNTHESIZER_OBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_TEST_SPEECH_SYNTHESIZER_OBSERVER_H_

#include <chrono>
#include <deque>
#include <mutex>
#include <condition_variable>

#include "SpeechSynthesizer/SpeechSynthesizerObserver.h"
#include "SpeechSynthesizer/SpeechSynthesizerState.h"

using namespace alexaClientSDK::capabilityAgents::speechSynthesizer;

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * Interface for observing a SpeechSynthesizer.
 */
class TestSpeechSynthesizerObserver : public SpeechSynthesizerObserver {
public:
    TestSpeechSynthesizerObserver();

    ~TestSpeechSynthesizerObserver() = default;

    void onStateChanged(SpeechSynthesizerState state) override;

    bool checkState(const SpeechSynthesizerState expectedState, const std::chrono::seconds duration);

    SpeechSynthesizerState waitForNext (const std::chrono::seconds duration);
    
    SpeechSynthesizerState getCurrentState();

private:
    SpeechSynthesizerState m_state;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
    std::deque<SpeechSynthesizerState> m_queue;
};

} // namespace test
} // namespace integration
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_TEST_SPEECH_SYNTHESIZER_OBSERVER_H_
