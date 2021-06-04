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
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "Settings/SharedAVSSettingProtocol.h"

/// String to identify log entries originating from this file.
static const std::string TAG("SharedAVSSettingProtocol");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace settings {

using namespace avsCommon::utils::metrics;

/// Protocol should call apply change with an empty string when no value is found in the database.
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

SharedAVSSettingProtocol::Request::Request(
    ApplyChangeFunction applyFn,
    RevertChangeFunction revertFn,
    SettingNotificationFunction notifyFn) :
        applyChange{applyFn},
        revertChange{revertFn},
        notifyObservers{notifyFn} {
}

std::unique_ptr<SharedAVSSettingProtocol> SharedAVSSettingProtocol::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    bool isDefaultCloudAuthoritative) {
    ACSDK_DEBUG5(LX(__func__).d("settingName", metadata.settingName));

    if (!eventSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullEventSender"));
        return nullptr;
    }

    if (!settingStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSettingStorage"));
        return nullptr;
    }

    if (!connectionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullConnectionManager"));
        return nullptr;
    }

    std::string settingKey = metadata.eventNamespace + "::" + metadata.settingName;

    return std::unique_ptr<SharedAVSSettingProtocol>(new SharedAVSSettingProtocol(
        settingKey, eventSender, settingStorage, connectionManager, metricRecorder, isDefaultCloudAuthoritative));
}

SharedAVSSettingProtocol::~SharedAVSSettingProtocol() {
    m_connectionManager->removeConnectionStatusObserver(m_connectionObserver);
}

SetSettingResult SharedAVSSettingProtocol::localChange(
    ApplyChangeFunction applyChange,
    RevertChangeFunction revertChange,
    SettingNotificationFunction notifyObservers) {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));

    if (!applyChange || !revertChange || !notifyObservers) {
        ACSDK_ERROR(LX("localChangeFailed")
                        .d("reason", "invalidCallback")
                        .d("invalidApply", !applyChange)
                        .d("invalidRevert", !revertChange)
                        .d("invalidNotify", !notifyObservers));
        return SetSettingResult::INTERNAL_ERROR;
    }

    std::lock_guard<std::mutex> lock{m_requestLock};
    bool executorReady = !m_pendingRequest;
    m_pendingRequest = std::unique_ptr<Request>(new Request(applyChange, revertChange, notifyObservers));

    if (executorReady) {
        m_executor.submit([this]() {
            std::unique_lock<std::mutex> lock(m_requestLock);
            auto request = std::move(m_pendingRequest);
            lock.unlock();
            // It is possible that the m_pendingRequest was reset in SharedAVSSettingProtocol::clearData()
            // Check that the request from it is not null.
            if (request == nullptr) {
                ACSDK_ERROR(LX("localChangeFailed").d("reason", "nullRequestPtr"));
                return;
            }

            request->notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS);

            bool ok;
            std::string value;
            std::tie(ok, value) = request->applyChange();
            if (!ok) {
                ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotApplyChange"));
                request->notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
                submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 1);
                return;
            }

            if (!this->m_storage->storeSetting(m_key, value, SettingStatus::LOCAL_CHANGE_IN_PROGRESS)) {
                ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotUpdateDatabase"));
                request->revertChange();
                request->notifyObservers(SettingNotifications::LOCAL_CHANGE_FAILED);
                submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 1);
                return;
            }

            request->notifyObservers(SettingNotifications::LOCAL_CHANGE);
            submitMetric(m_metricRecorder, LOCAL_CHANGE_METRIC, m_key, 1);
            submitMetric(m_metricRecorder, LOCAL_CHANGE_FAILED_METRIC, m_key, 0);

            if (!this->m_eventSender->sendChangedEvent(value).get()) {
                ACSDK_ERROR(LX("localChangeFailed").d("reason", "sendEventFailed"));
                return;
            }

            if (!this->m_storage->updateSettingStatus(m_key, SettingStatus::SYNCHRONIZED)) {
                ACSDK_ERROR(LX("localChangeFailed").d("reason", "cannotUpdateStatus"));
            }
        });
    }

    return SetSettingResult::ENQUEUED;
}
bool SharedAVSSettingProtocol::avsChange(
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

    std::lock_guard<std::mutex> lock{m_requestLock};
    bool executorReady = !m_pendingRequest;

    m_pendingRequest = std::unique_ptr<Request>(new Request(applyChange, revertChange, notifyObservers));

    if (executorReady) {
        m_executor.submit([this]() {
            std::unique_lock<std::mutex> lock(m_requestLock);
            auto request = std::move(m_pendingRequest);
            lock.unlock();
            // It is possible that the m_pendingRequest was reset in SharedAVSSettingProtocol::clearData()
            // Check that the request from it is not null.
            if (request == nullptr) {
                ACSDK_ERROR(LX("avsChangeFailed").d("reason", "nullRequestPtr"));
                return;
            }

            request->notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS);

            bool ok;
            std::string value;
            std::tie(ok, value) = request->applyChange();

            if (!ok) {
                ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotApplyChange"));
                request->notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
                submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 1);
            } else if (!this->m_storage->storeSetting(m_key, value, SettingStatus::AVS_CHANGE_IN_PROGRESS)) {
                ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotUpdateDatabaseValue"));
                request->notifyObservers(SettingNotifications::AVS_CHANGE_FAILED);
                value = request->revertChange();
                submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 1);
            } else {
                request->notifyObservers(SettingNotifications::AVS_CHANGE);
                submitMetric(m_metricRecorder, AVS_CHANGE_FAILED_METRIC, m_key, 0);
            }
            submitMetric(m_metricRecorder, AVS_CHANGE_METRIC, m_key, 1);

            /// We need to send the report for failure or success case.
            if (!this->m_eventSender->sendReportEvent(value).get()) {
                ACSDK_ERROR(LX("avsChangeFailed").d("reason", "sendEventFailed"));
                return;
            }

            if (!this->m_storage->updateSettingStatus(m_key, SettingStatus::SYNCHRONIZED)) {
                ACSDK_ERROR(LX("avsChangeFailed").d("reason", "cannotUpdateStatus"));
            }
        });
    }
    return true;
}

bool SharedAVSSettingProtocol::restoreValue(
    ApplyDbChangeFunction applyChange,
    SettingNotificationFunction notifyObservers) {
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
    SettingStatus status;
    std::tie(status, valueOrErrorStr) = m_storage->loadSetting(m_key);

    if (SettingStatus::NOT_AVAILABLE != status) {
        valueStr = valueOrErrorStr;
    }

    auto applyStrChange = [valueStr, applyChange] { return applyChange(valueStr); };
    auto revertChange = [applyChange] { return applyChange(INVALID_VALUE).second; };
    switch (status) {
        case SettingStatus::NOT_AVAILABLE:
            if (m_isDefaultCloudAuthoritative) {
                return avsChange(applyStrChange, revertChange, notifyObservers);
            } else {
                return localChange(applyStrChange, revertChange, notifyObservers) == SetSettingResult::ENQUEUED;
            }
            break;
        case SettingStatus::LOCAL_CHANGE_IN_PROGRESS:
            return localChange(applyStrChange, revertChange, notifyObservers) == SetSettingResult::ENQUEUED;
        case SettingStatus::AVS_CHANGE_IN_PROGRESS:
            return avsChange(applyStrChange, revertChange, notifyObservers);
        case SettingStatus::SYNCHRONIZED:
            return applyChange(valueStr).first;
    }
    return false;
}

bool SharedAVSSettingProtocol::clearData() {
    ACSDK_DEBUG5(LX(__func__).d("setting", m_key));
    std::lock_guard<std::mutex> lock{m_requestLock};
    m_pendingRequest.reset();
    return m_storage->deleteSetting(m_key);
}

SharedAVSSettingProtocol::SharedAVSSettingProtocol(
    const std::string& key,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    bool isDefaultCloudAuthoritative) :
        m_key{key},
        m_isDefaultCloudAuthoritative{isDefaultCloudAuthoritative},
        m_eventSender{eventSender},
        m_storage{settingStorage},
        m_metricRecorder{metricRecorder} {
    m_connectionManager = connectionManager;

    m_connectionObserver = SettingConnectionObserver::create(
        std::bind(&SharedAVSSettingProtocol::connectionStatusChangeCallback, this, std::placeholders::_1));

    m_connectionManager->addConnectionStatusObserver(m_connectionObserver);
}

void SharedAVSSettingProtocol::executeSynchronizeOnConnected() {
    std::shared_future<bool> synchronizedFuture;

    std::string value;

    auto storedSetting = m_storage->loadSetting(m_key);
    value = storedSetting.second;
    SettingStatus status = storedSetting.first;

    ACSDK_DEBUG5(LX(__func__)
                     .d("setting", m_key)
                     .d("synchronized", SettingStatus::SYNCHRONIZED == status)
                     .sensitive("value", value));

    if (SettingStatus::LOCAL_CHANGE_IN_PROGRESS == status) {
        synchronizedFuture = m_eventSender->sendChangedEvent(value);
    } else if (SettingStatus::AVS_CHANGE_IN_PROGRESS == status) {
        synchronizedFuture = m_eventSender->sendReportEvent(value);
    } else {
        ACSDK_DEBUG5(LX(__func__).d("result", "alreadySynchronized"));
        return;
    }

    if (synchronizedFuture.get()) {
        if (!m_storage->updateSettingStatus(m_key, SettingStatus::SYNCHRONIZED)) {
            ACSDK_ERROR(LX("synchronizeFailed").d("reason", "cannotUpdateStatus"));
        }
    } else {
        ACSDK_ERROR(LX("synchronizeFailed").d("reason", "sendEventFailed").d("status", static_cast<int>(status)));
    }
}

void SharedAVSSettingProtocol::connectionStatusChangeCallback(bool isConnected) {
    if (isConnected) {
        m_executor.submit(std::bind(&SharedAVSSettingProtocol::executeSynchronizeOnConnected, this));
    }
}

}  // namespace settings
}  // namespace alexaClientSDK
