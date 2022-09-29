/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "DefaultClient/ConnectionRetryTrigger.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "ConnectionRetryTrigger"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::sdkInterfaces;
using namespace capabilityAgents::aip;

std::shared_ptr<ConnectionRetryTrigger> ConnectionRetryTrigger::create(
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> audioInputProcessor) {
    if (!connectionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullConnectionManager"));
        return nullptr;
    }

    if (!audioInputProcessor) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAudioInputProcessor"));
        return nullptr;
    }

    std::shared_ptr<ConnectionRetryTrigger> connectionRetryTrigger(new ConnectionRetryTrigger(connectionManager));
    audioInputProcessor->addObserver(connectionRetryTrigger);
    return connectionRetryTrigger;
}

ConnectionRetryTrigger::ConnectionRetryTrigger(
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager) :
        m_state{AudioInputProcessorObserverInterface::State::IDLE},
        m_connectionManager{connectionManager} {
}

void ConnectionRetryTrigger::onStateChanged(AudioInputProcessorObserverInterface::State state) {
    ACSDK_DEBUG9(LX(__func__).d("state", state));
    if (AudioInputProcessorObserverInterface::State::IDLE == m_state && state != m_state) {
        m_connectionManager->onWakeConnectionRetry();
    }
    m_state = state;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
