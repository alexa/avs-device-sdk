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

#include "acsdkDeviceSetup/DeviceSetupMessageRequest.h"

namespace alexaClientSDK {
namespace acsdkDeviceSetup {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"DeviceSetupMessageRequest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Helper function to determine whether the status is deemed successful.
 *
 * @param status The status.
 * @param Whether the status is deemed successful.
 */
static bool isCompletedSuccessfully(MessageRequestObserverInterface::Status status) {
    switch (status) {
        case MessageRequestObserverInterface::Status::SUCCESS:
        case MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED:
        case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
            return true;
        case MessageRequestObserverInterface::Status::PENDING:
        case MessageRequestObserverInterface::Status::THROTTLED:
        case MessageRequestObserverInterface::Status::NOT_CONNECTED:
        case MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED:
        case MessageRequestObserverInterface::Status::TIMEDOUT:
        case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
        case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
        case MessageRequestObserverInterface::Status::REFUSED:
        case MessageRequestObserverInterface::Status::CANCELED:
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
        case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
            return false;
        default:
            ACSDK_ERROR(LX(__func__).m("status not found").d("status", status));
            return false;
    }
}

DeviceSetupMessageRequest::DeviceSetupMessageRequest(
    const std::string& jsonContent,
    std::promise<bool> messageCompletePromise) :
        MessageRequest{jsonContent},
        m_isPromiseSet{false},
        m_messageCompletePromise{std::move(messageCompletePromise)} {
}

void DeviceSetupMessageRequest::sendCompleted(MessageRequestObserverInterface::Status status) {
    MessageRequest::sendCompleted(status);

    if (!m_isPromiseSet.exchange(true)) {
        bool success = isCompletedSuccessfully(status);
        ACSDK_DEBUG5(LX(__func__).d("status", status).d("success", success));
        m_messageCompletePromise.set_value(success);
    } else {
        // Should never be called multiple times.
        ACSDK_ERROR(LX("setPromiseFailed").d("reason", "promiseAlreadySet").d("status", status));
    }
}

}  // namespace acsdkDeviceSetup
}  // namespace alexaClientSDK
