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

#include <functional>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>

#include "acsdkDoNotDisturb/DNDSettingProtocol.h"

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

using namespace avsCommon::utils::metrics;
using namespace settings;

/// String to designate invalid value for the DND setting. NOTE: Valid values are "true" and "false".
static const std::string INVALID_VALUE = "";

/// The metrics source string.
static const std::string METRIC_SOURCE_PREFIX = "SETTINGS-";

/// The local change metric string.
static const std::string LOCAL_CHANGE_METRIC = "LOCAL_CHANGE";

/// The local change failed metric string.
static const std::string LOCAL_CHANGE_FAILED_METRIC = "LOCAL_CHANGE_FAILED";

/// The AVS change metric string.
static const std::string AVS_CHANGE_METRIC = "AVS_CHANGE";

/// The local change failed metric string.
static const std::string AVS_CHANGE_FAILED_METRIC = "AVS_CHANGE_FAILED";

/// The setting key metric string.
static const std::string SETTING_KEY = "SETTING_KEY";

/**
 * Submits a counter metric with the passed in event name
 * @note: This function returns immediately if metricRecorder is null.
 *
 * @param metricRecorder The @c MetricRecorder instance used to record the metric.
 * @param eventName The event name string.
 * @param count The count to increment the datapoint with.
 */
static void submitMetric(
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::string& eventName,
    const std::string& settingsKey,
    uint64_t count) {
    if (!metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(METRIC_SOURCE_PREFIX + eventName)
                           .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(count).build())
                           .addDataPoint(DataPointStringBuilder{}.setName(SETTING_KEY).setValue(settingsKey).build())
                           .build();

    if (!metricEvent) {
        ACSDK_ERROR(LX("submitMetricFailed").d("reason", "invalid metric event"));
        return;
    }

    recordMetric(metricRecorder, metricEvent);
}

std::unique_ptr<DNDSettingProtocol> DNDSettingProtocol::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
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

    return std::unique_ptr<DNDSettingProtocol>(
        new DNDSettingProtocol(settingKey, eventSender, settingStorage, metricRecorder));
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
        notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS);

        bool ok = false;
        std::string value;
        std::tie(ok, value) = applyChange();
        if (!ok) {
            ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotApplyChange"));
            notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
            submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 1);
            return;
        }

        if (!this->m_storage->storeSetting(m_key, value, SettingStatus::LOCAL_CHANGE_IN_PROGRESS)) {
            ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotUpdateDatabase"));
            revertChange();
            notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
            submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 1);
            return;
        }

        notifyObservers(SettingNotifications::LOCAL_CHANGE);
        submitMetric(m_metricRecorder, LOCAL_CHANGE_METRIC, m_key, 1);
        submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 0);

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

        notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS);

        bool ok = false;
        std::string value;
        std::tie(ok, value) = applyChange();
        if (!ok) {
            ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotApplyChange"));
            notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
            submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 1);
        } else if (!this->m_storage->storeSetting(m_key, value, SettingStatus::AVS_CHANGE_IN_PROGRESS)) {
            ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotUpdateDatabaseValue"));
            notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
            value = revertChange();
            submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 1);
        } else {
            notifyObservers(SettingNotifications::AVS_CHANGE);
            submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 0);
        }

        submitMetric(m_metricRecorder, AVS_CHANGE_METRIC, m_key, 1);

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

bool DNDSettingProtocol::clearData() {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));
    return m_storage->deleteSetting(m_key);
}

DNDSettingProtocol::DNDSettingProtocol(
    const std::string& key,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) :
        m_key{key},
        m_eventSender{eventSender},
        m_storage{settingStorage},
        m_metricRecorder{metricRecorder} {
}

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
