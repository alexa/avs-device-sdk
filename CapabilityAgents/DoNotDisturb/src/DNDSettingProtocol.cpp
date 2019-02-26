/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <functional>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "DoNotDisturbCA/DNDSettingProtocol.h"

/// String to identify log entries originating from this file.
static const std::string TAG("DNDSettingProtocol");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

using namespace settings;

/// String to designate invalid value for the DND setting. NOTE: Valid values are "true" and "false".
static const std::string INVALID_VALUE = "";

std::unique_ptr<DNDSettingProtocol> DNDSettingProtocol::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage) {
    ACSDK_DEBUG5(LX(__func__).d("settingName", metadata.settingName));

    if (!eventSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullEventSender"));
        return nullptr;
    }

    if (!settingStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSettingStorage"));
        return nullptr;
    }

    std::string settingKey = metadata.eventNamespace + "::" + metadata.settingName;

    return std::unique_ptr<DNDSettingProtocol>(new DNDSettingProtocol(settingKey, eventSender, settingStorage));
}

SetSettingResult DNDSettingProtocol::localChange(
    ApplyChangeFunction applyChange,
    RevertChangeFunction revertChange,
    SettingNotificationFunction notifyObservers) {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));

    if (!applyChange || !revertChange || !notifyObservers) {
        ACSDK_ERROR(LX("avsChangeFailed")
                        .d("reason", "invalidCallback")
                        .d("invalidApply", !applyChange)
                        .d("invalidRevert", !revertChange)
                        .d("invalidNotify", !notifyObservers));
        return SetSettingResult::INTERNAL_ERROR;
    }

    m_executor.submit([this, applyChange, revertChange, notifyObservers]() {
        bool ok = false;
        std::string value;
        std::tie(ok, value) = applyChange();
        if (!ok) {
            ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotApplyChange"));
            notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
            return;
        }

        if (!this->m_storage->storeSetting(m_key, value, SettingStatus::LOCAL_CHANGE_IN_PROGRESS)) {
            ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotUpdateDatabase"));
            revertChange();
            notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
            return;
        }

        notifyObservers(SettingNotifications::LOCAL_CHANGE);

        this->m_eventSender->sendChangedEvent(value).get();

        if (!this->m_storage->storeSetting(m_key, value, SettingStatus::SYNCHRONIZED)) {
            ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotUpdateStatus"));
        }
    });

    return SetSettingResult::ENQUEUED;
}

bool DNDSettingProtocol::avsChange(
    ApplyChangeFunction applyChange,
    RevertChangeFunction revertChange,
    SettingNotificationFunction notifyObservers) {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));

    if (!applyChange || !revertChange || !notifyObservers) {
        ACSDK_ERROR(LX("avsChangeFailed")
                        .d("reason", "invalidCallback")
                        .d("invalidApply", !applyChange)
                        .d("invalidRevert", !revertChange)
                        .d("invalidNotify", !notifyObservers));
        return false;
    }

    std::promise<bool> requestSaved;
    auto future = requestSaved.get_future();
    m_executor.submit([this, applyChange, revertChange, notifyObservers, &requestSaved]() {
        // Log request before setting the value for recovery.
        if (!m_storage->updateSettingStatus(m_key, SettingStatus::AVS_CHANGE_IN_PROGRESS)) {
            requestSaved.set_value(false);
            return;
        }
        requestSaved.set_value(true);

        bool ok = false;
        std::string value;
        std::tie(ok, value) = applyChange();
        if (!ok) {
            ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotApplyChange"));
            notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
        } else if (!this->m_storage->storeSetting(m_key, value, SettingStatus::AVS_CHANGE_IN_PROGRESS)) {
            ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotUpdateDatabaseValue"));
            notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
            value = revertChange();
        } else {
            notifyObservers(SettingNotifications::AVS_CHANGE);
        }

        /// We need to sent the report for failure or success case.
        this->m_eventSender->sendReportEvent(value);

        if (!this->m_storage->updateSettingStatus(m_key, SettingStatus::SYNCHRONIZED)) {
            ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotUpdateStatus"));
        }
    });
    return future.get();
}

bool DNDSettingProtocol::restoreValue(ApplyDbChangeFunction applyChange, SettingNotificationFunction notifyObservers) {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));

    if (!applyChange || !notifyObservers) {
        ACSDK_ERROR(LX("avsChangeFailed")
                        .d("reason", "invalidCallback")
                        .d("invalidApply", !applyChange)
                        .d("invalidNotify", !notifyObservers));
        return false;
    }

    std::string valueOrErrorStr;
    std::string valueStr;
    SettingStatus status = SettingStatus::NOT_AVAILABLE;
    std::tie(status, valueOrErrorStr) = m_storage->loadSetting(m_key);
    if (SettingStatus::NOT_AVAILABLE != status) {
        valueStr = valueOrErrorStr;
    }
    auto applyStrChange = [valueStr, applyChange] { return applyChange(valueStr); };
    auto revertChange = [applyChange] { return applyChange(INVALID_VALUE).second; };
    switch (status) {
        case SettingStatus::NOT_AVAILABLE:
            // Fall-through
        case SettingStatus::LOCAL_CHANGE_IN_PROGRESS:
            return localChange(applyStrChange, revertChange, notifyObservers) == SetSettingResult::ENQUEUED;
        case SettingStatus::AVS_CHANGE_IN_PROGRESS:
            return avsChange(applyStrChange, revertChange, notifyObservers);
        case SettingStatus::SYNCHRONIZED:
            return applyChange(valueStr).first;
    }
    return false;
}

DNDSettingProtocol::DNDSettingProtocol(
    const std::string& key,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage) :
        m_key{key},
        m_eventSender{eventSender},
        m_storage{settingStorage} {
}

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
