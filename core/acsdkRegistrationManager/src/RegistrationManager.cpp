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

#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "RegistrationManager/RegistrationManager.h"

namespace alexaClientSDK {
namespace registrationManager {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::metrics;

/// The metric source prefix string.
static const std::string METRIC_SOURCE_PREFIX = "REGISTRATION_MANAGER-";

/// The logout occurred metric string.
static const std::string LOGOUT_OCCURRED = "LOGOUT_OCCURRED";

/// String to identify log entries originating from this file.
#define TAG "RegistrationManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Submits a logout occurred metric to the metric recorder.
 * @param metricRecorder - The @c MetricRecorderInterface pointer.
 */
static void submitLogoutMetric(const std::shared_ptr<MetricRecorderInterface>& metricRecorder) {
    if (!metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(METRIC_SOURCE_PREFIX + LOGOUT_OCCURRED)
                           .addDataPoint(DataPointCounterBuilder{}.setName(LOGOUT_OCCURRED).increment(1).build())
                           .build();

    if (!metricEvent) {
        ACSDK_ERROR(LX("submitLogoutMetricFailed").d("reason", "null metric event"));
        return;
    }

    recordMetric(metricRecorder, metricEvent);
}

std::shared_ptr<registrationManager::RegistrationManagerInterface> RegistrationManager::
    createRegistrationManagerInterface(
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& dataManager,
        const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier,
        const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    if (!dataManager) {
        ACSDK_ERROR(LX("createRegistrationManagerInterfaceFailed").d("reason", "invalid data manager"));
    } else if (!notifier) {
        ACSDK_ERROR(LX("createRegistrationManagerInterfaceFailed").d("reason", "invalid notifier"));
    } else if (!connectionManager) {
        ACSDK_ERROR(LX("createRegistrationManagerInterfaceFailed").d("reason", "invalid connection manager"));
    } else if (!directiveSequencer) {
        ACSDK_ERROR(LX("createRegistrationManagerInterfaceFailed").d("reason", "invalid directive sequencer"));
    } else {
        return std::shared_ptr<RegistrationManagerInterface>(
            new RegistrationManager(dataManager, notifier, connectionManager, directiveSequencer, metricRecorder));
    }

    return nullptr;
}

RegistrationManager::RegistrationManager(
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& dataManager,
    const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) :
        m_dataManager{dataManager},
        m_notifier{notifier},
        m_connectionManager{connectionManager},
        m_directiveSequencer{directiveSequencer},
        m_metricRecorder{metricRecorder} {
}

void RegistrationManager::logout() {
    ACSDK_INFO(LX(__func__));
    m_directiveSequencer->disable();
    m_connectionManager->disable();
    m_dataManager->clearData();
    m_notifier->notifyObservers([](const std::shared_ptr<RegistrationObserverInterface>& observer) {
        if (observer) {
            observer->onLogout();
        }
    });
    submitLogoutMetric(m_metricRecorder);
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
