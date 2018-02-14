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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AIPSTATEOBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AIPSTATEOBSERVER_H_

#include <chrono>
#include <deque>
#include <condition_variable>
#include <mutex>

#include <AVSCommon/SDKInterfaces/AudioInputProcessorObserverInterface.h>

namespace alexaClientSDK {
namespace integration {

class AipStateObserver : public avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface {
public:
    AipStateObserver();
    void onStateChanged(avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State newState) override;
    bool checkState(
        const avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State expectedState,
        const std::chrono::seconds duration = std::chrono::seconds(10));
    avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State waitForNext(
        const std::chrono::seconds duration);

private:
    avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State m_state;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
    std::deque<avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State> m_queue;
};

}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AIPSTATEOBSERVER_H_
