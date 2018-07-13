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

#include "AVSCommon/Utils/Logger/Logger.h"
#include "RegistrationManager/CustomerDataManager.h"
#include "RegistrationManager/CustomerDataHandler.h"

/// String to identify log entries originating from this file.
static const std::string TAG("CustomerDataHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace registrationManager {

CustomerDataHandler::CustomerDataHandler(std::shared_ptr<CustomerDataManager> manager) : m_dataManager{manager} {
    if (!manager) {
        ACSDK_ERROR(
            LX(__func__).m("Failed to register CustomerDataHandler. The customer data manager provided is "
                           "invalid."));
    } else {
        m_dataManager->addDataHandler(this);
    }
}

CustomerDataHandler::~CustomerDataHandler() {
    if (!m_dataManager) {
        ACSDK_ERROR(LX(__func__).m("Failed to remove CustomerDataHandler. Customer data manager is invalid."));
    } else {
        m_dataManager->removeDataHandler(this);
    }
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
