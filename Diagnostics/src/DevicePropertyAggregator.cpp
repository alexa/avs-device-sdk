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

#include "Diagnostics/DevicePropertyAggregator.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace diagnostics {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG{"DevicePropertyAggregator"};

/// String to identify invalid property value.
static const std::string INVALID_VALUE{"INVALID"};

/// String to identify IDLE property value.
static const std::string IDLE{"IDLE"};

/// String to identify NONE property value.
static const std::string NONE{"NONE"};

/// Timeout value to wait for asynchronous operations.
static const std::chrono::seconds TIMEOUT{2};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<DevicePropertyAggregator> DevicePropertyAggregator::create() {
    return std::shared_ptr<DevicePropertyAggregator>(new DevicePropertyAggregator());
}

DevicePropertyAggregator::DevicePropertyAggregator() {
    initializePropertyMap();
}

void DevicePropertyAggregator::setContextManager(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    ACSDK_DEBUG5(LX(__func__));

    m_contextManager = contextManager;
}

void DevicePropertyAggregator::initializeVolume(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (!speakerManager) {
        ACSDK_ERROR(LX("initializeVolumeFailed").d("reason", "nullptr"));
        return;
    }

    SpeakerInterface::SpeakerSettings settings;
    auto result = speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    if (std::future_status::ready == result.wait_for(TIMEOUT) && result.get()) {
        updateSpeakerSettingsInPropertyMap(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, settings);
    }

    result = speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, &settings);
    if (std::future_status::ready == result.wait_for(TIMEOUT) && result.get()) {
        updateSpeakerSettingsInPropertyMap(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, settings);
    }
}

void DevicePropertyAggregator::initializePropertyMap() {
    m_propertyMap[DevicePropertyAggregatorInterface::TTS_PLAYER_STATE] = IDLE;
    m_propertyMap[DevicePropertyAggregatorInterface::AUDIO_PLAYER_STATE] = IDLE;
    m_propertyMap[DevicePropertyAggregatorInterface::CONTENT_ID] = NONE;
    m_propertyMap[DevicePropertyAggregatorInterface::ALERT_TYPE_AND_STATE] = IDLE;

    auto contextOptional = getDeviceContextJson();
    if (contextOptional.hasValue()) {
        m_propertyMap[DevicePropertyAggregatorInterface::DEVICE_CONTEXT] = contextOptional.value();
    }
}

std::unordered_map<std::string, std::string> DevicePropertyAggregator::getAllDeviceProperties() {
    auto future = m_executor.submit([this]() { return m_propertyMap; });

    return future.get();
}

Optional<std::string> DevicePropertyAggregator::getDeviceProperty(const std::string& propertyKey) {
    auto maybePropertyValueFuture = m_executor.submit([this, propertyKey]() {
        Optional<std::string> maybePropertyValue;

        if (DevicePropertyAggregatorInterface::DEVICE_CONTEXT == propertyKey) {
            maybePropertyValue = getDeviceContextJson();
        } else {
            auto it = m_propertyMap.find(propertyKey);
            if (it != m_propertyMap.end()) {
                maybePropertyValue = it->second;
            }
        }

        return maybePropertyValue;
    });

    auto maybePropertyValue = maybePropertyValueFuture.get();
    if (maybePropertyValue.hasValue()) {
        ACSDK_DEBUG5(LX(__func__).d("propertyKey", propertyKey).d("propertyValue", maybePropertyValue.value()));
    } else {
        ACSDK_DEBUG5(LX(__func__).d("propertyKey", propertyKey).m("unknown property"));
    }

    return maybePropertyValue;
}

void DevicePropertyAggregator::onContextFailure(const ContextRequestError error) {
    ACSDK_ERROR(LX(__func__).d("reason", error));
    m_contextWakeTrigger.notify_all();
}

void DevicePropertyAggregator::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_deviceContextMutex);
    m_deviceContext.set(jsonContext);
    m_contextWakeTrigger.notify_all();
}

Optional<std::string> DevicePropertyAggregator::getDeviceContextJson() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_contextManager) {
        Optional<std::string> empty;
        return empty;
    }

    std::unique_lock<std::mutex> lock(m_deviceContextMutex);
    m_deviceContext.reset();
    m_contextManager->getContext(shared_from_this());
    m_contextWakeTrigger.wait_for(lock, TIMEOUT, [this]() { return m_deviceContext.hasValue(); });
    return m_deviceContext;
}

void DevicePropertyAggregator::onAlertStateChange(
    const std::string& alertToken,
    const std::string& alertType,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, alertType, state]() {
        std::stringstream ss;
        ss << alertType << ":" << state;
        m_propertyMap[DevicePropertyAggregatorInterface::ALERT_TYPE_AND_STATE] = ss.str();
    });
}

void DevicePropertyAggregator::onPlayerActivityChanged(
    PlayerActivity state,
    const AudioPlayerObserverInterface::Context& context) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, state, context]() {
        std::string playerActivityState = avsCommon::avs::playerActivityToString(state);
        m_propertyMap[DevicePropertyAggregatorInterface::AUDIO_PLAYER_STATE] = playerActivityState;
        m_propertyMap[DevicePropertyAggregatorInterface::CONTENT_ID] = context.audioItemId;
    });
}

void DevicePropertyAggregator::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, status]() {
        std::stringstream ss;
        ss << status;
        m_propertyMap[DevicePropertyAggregatorInterface::CONNECTION_STATE] = ss.str();
    });
}

void DevicePropertyAggregator::onSetIndicator(IndicatorState state) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, state]() {
        std::stringstream ss;
        ss << state;
        m_propertyMap[DevicePropertyAggregatorInterface::NOTIFICATION_INDICATOR] = ss.str();
    });
}

void DevicePropertyAggregator::onNotificationReceived() {
    // No-op.
}

void DevicePropertyAggregator::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, newState]() {
        m_propertyMap[DevicePropertyAggregatorInterface::TTS_PLAYER_STATE] =
            DialogUXStateObserverInterface::stateToString(newState);
    });
}

void DevicePropertyAggregator::onSpeakerSettingsChanged(
    const SpeakerManagerObserverInterface::Source& source,
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, type, settings]() { updateSpeakerSettingsInPropertyMap(type, settings); });
}

void DevicePropertyAggregator::updateSpeakerSettingsInPropertyMap(
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__));
    std::string volumeAsString = std::to_string(settings.volume);

    std::stringstream ss;
    ss << std::boolalpha << settings.mute;
    std::string isMuteAsString = ss.str();

    switch (type) {
        case ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME:
            m_propertyMap[DevicePropertyAggregatorInterface::AVS_SPEAKER_VOLUME] = volumeAsString;
            m_propertyMap[DevicePropertyAggregatorInterface::AVS_SPEAKER_MUTE] = isMuteAsString;
            break;
        case ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME:
            m_propertyMap[DevicePropertyAggregatorInterface::AVS_ALERTS_VOLUME] = volumeAsString;
            m_propertyMap[DevicePropertyAggregatorInterface::AVS_ALERTS_MUTE] = isMuteAsString;
            break;
    }
}

}  // namespace diagnostics
}  // namespace alexaClientSDK
