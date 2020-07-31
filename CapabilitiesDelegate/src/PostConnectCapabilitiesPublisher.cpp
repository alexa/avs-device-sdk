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

#include "CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h"
#include "CapabilitiesDelegate/Utils/DiscoveryUtils.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <CapabilitiesDelegate/DiscoveryEventSender.h>

#include <utility>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace capabilitiesDelegate::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectCapabilitiesPublisher");

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PostConnectCapabilitiesPublisher> PostConnectCapabilitiesPublisher::create(
    const std::shared_ptr<DiscoveryEventSenderInterface>& discoveryEventSender) {
    if (!discoveryEventSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalid DiscoveryEventSender"));
    } else {
        auto instance = std::shared_ptr<PostConnectCapabilitiesPublisher>(
            new PostConnectCapabilitiesPublisher(discoveryEventSender));

        return instance;
    }
    return nullptr;
}

PostConnectCapabilitiesPublisher::PostConnectCapabilitiesPublisher(
    std::shared_ptr<DiscoveryEventSenderInterface> discoveryEventSender) :
        m_isPerformOperationInvoked{false},
        m_discoveryEventSender{std::move(discoveryEventSender)} {
}

PostConnectCapabilitiesPublisher::~PostConnectCapabilitiesPublisher() {
    ACSDK_DEBUG5(LX(__func__));
    m_discoveryEventSender->stop();
}

unsigned int PostConnectCapabilitiesPublisher::getOperationPriority() {
    return ENDPOINT_DISCOVERY_PRIORITY;
}

bool PostConnectCapabilitiesPublisher::performOperation(const std::shared_ptr<MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!messageSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "nullPostConnectSender"));
        return false;
    }

    {
        /// Ensure that performOperation method cannot be called twice in a row.
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isPerformOperationInvoked) {
            ACSDK_ERROR(LX("performOperationFailed").d("reason", "performOperation should only be called once."));
            return false;
        }
        m_isPerformOperationInvoked = true;
    }

    if (!m_discoveryEventSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "DiscoveryEventSender is null"));
        return false;
    }
    return m_discoveryEventSender->sendDiscoveryEvents(messageSender);
}

void PostConnectCapabilitiesPublisher::abortOperation() {
    ACSDK_DEBUG5(LX(__func__));
    m_discoveryEventSender->stop();
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
