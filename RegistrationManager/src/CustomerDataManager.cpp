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

#include "RegistrationManager/CustomerDataManager.h"

#include <mutex>

#include "AVSCommon/Utils/Logger/Logger.h"

/// String to identify log entries originating from this file.
static const std::string TAG("CustomerDataManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace registrationManager {

void CustomerDataManager::addDataHandler(CustomerDataHandler* handler) {
    if (handler == nullptr) {
        ACSDK_ERROR(LX("addDataHandlerFailed").m("Cannot register a NULL handler."));
    } else {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_dataHandlers.insert(handler);
    }
}

void CustomerDataManager::removeDataHandler(CustomerDataHandler* handler) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_dataHandlers.erase(handler);
}

void CustomerDataManager::clearData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto handler : m_dataHandlers) {
        handler->clearData();
    }
}

CustomerDataManager::~CustomerDataManager() {
    if (!m_dataHandlers.empty()) {
        ACSDK_ERROR(LX(__func__).m("All CustomerDataHandlers should be removed before deleting their manager."));
    }
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
