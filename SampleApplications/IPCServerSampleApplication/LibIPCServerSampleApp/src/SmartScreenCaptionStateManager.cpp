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

#include "IPCServerSampleApp/SmartScreenCaptionStateManager.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/LogEntry.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

static const std::string TAG{"SmartScreenCaptionStateManager"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// Component name for IPCServerSampleApp
static const std::string componentName = "IPCServerSampleApp";

/// Table name for settings
static const std::string tableName = "Settings";

/// Key for Captions setting
static const std::string captionsKey = "CaptionsEnabled";

/// Value for Captions ON
static const std::string captionsEnabledString = "CAPTIONS_ENABLED";

/// Value for Captions OFF
static const std::string captionsDisabledString = "CAPTIONS_DISABLED";

SmartScreenCaptionStateManager::SmartScreenCaptionStateManager(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) :
        m_miscStorage{std::move(miscStorage)} {
    bool doesTableExist = false;
    if (!m_miscStorage->tableExists(componentName, tableName, &doesTableExist)) {
        ACSDK_ERROR(LX("checkIfSettingsTableExistsFailed").d("reason", "storageFailure"));
    }

    if (!doesTableExist) {
        if (!m_miscStorage->createTable(
                componentName,
                tableName,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::KeyType::STRING_KEY,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("createSettingsTableFailed").d("reason", "storageFailure"));
        }
    }
}

bool SmartScreenCaptionStateManager::areCaptionsEnabled() {
    std::string captionStatusValue;
    if (m_miscStorage->get(componentName, tableName, captionsKey, &captionStatusValue)) {
        return captionStatusValue == captionsEnabledString;
    } else {
        ACSDK_ERROR(LX("readCaptionsSettingFailed").d("reason", "storageFailure"));
        return false;
    }
}

void SmartScreenCaptionStateManager::setCaptionsState(bool enabled) {
    if (!m_miscStorage->put(
            componentName, tableName, captionsKey, enabled ? captionsEnabledString : captionsDisabledString)) {
        ACSDK_ERROR(LX("changeCaptionsStateFailed").d("reason", "storageFailure"));
    }
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK