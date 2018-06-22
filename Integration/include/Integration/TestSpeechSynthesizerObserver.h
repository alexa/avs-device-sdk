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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTSPEECHSYNTHESIZEROBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTSPEECHSYNTHESIZEROBSERVER_H_

#include <chrono>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <AVSCommon/SDKInterfaces/SpeechSynthesizerObserverInterface.h>

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * Interface for observing a SpeechSynthesizer.
 */
class TestSpeechSynthesizerObserver : public avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface {
public:
    TestSpeechSynthesizerObserver();

    ~TestSpeechSynthesizerObserver() = default;

    void onStateChanged(
        avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState state) override;

    bool checkState(
        const avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState expectedState,
        const std::chrono::seconds duration);

    avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState waitForNext(
        const std::chrono::seconds duration);

    avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState getCurrentState();

private:
    avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState m_state;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
    std::deque<avsCommon::sdkInterfaces::SpeechSynthesizerObserverInterface::SpeechSynthesizerState> m_queue;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTSPEECHSYNTHESIZEROBSERVER_H_
