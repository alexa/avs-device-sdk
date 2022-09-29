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
#define TAG "DevicePropertyAggregator"

/// String to identify invalid property value.
static const std::string INVALID_VALUE{"INVALID"};

/// String to identify IDLE property value.
static const std::string IDLE{"IDLE"};

/// String to identify NONE property value.
static const std::string NONE{"NONE"};

/// Timeout value to wait for asynchronous operations.
static const std::chrono::seconds TIMEOUT{2};

/// The list of Synchronous Properties that need to be retrieved before returning all device properties.
static const std::set<std::string> listOfSynchronousProperties = {
    DevicePropertyAggregator::DevicePropertyAggregatorInterface::DO_NOT_DISTURB,
    DevicePropertyAggregator::DevicePropertyAggregatorInterface::LOCALE,
    DevicePropertyAggregator::DevicePropertyAggregatorInterface::WAKE_WORDS};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Standard method to convert bool to string.
 *
 * @param value The boolean value.
 * @return The string representation.
 */
static std::string toString(bool value) {
    std::stringstream ss;
    ss << std::boolalpha << value;
    return ss.str();
}

std::shared_ptr<DevicePropertyAggregator> DevicePropertyAggregator::create() {
    return std::shared_ptr<DevicePropertyAggregator>(new DevicePropertyAggregator());
}

DevicePropertyAggregator::DevicePropertyAggregator() {
    initializeAsyncPropertyMap();
}

void DevicePropertyAggregator::setContextManager(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    ACSDK_DEBUG5(LX(__func__));

    m_contextManager = contextManager;
}

void DevicePropertyAggregator::setDeviceSettingsManager(
    std::shared_ptr<settings::DeviceSettingsManager> settingManager) {
    ACSDK_DEBUG5(LX(__func__));
    m_deviceSettingsManager = settingManager;
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

void DevicePropertyAggregator::onAuthStateChange(
    AuthObserverInterface::State newState,
    AuthObserverInterface::Error error) {
    ACSDK_DEBUG5(LX(__func__).d("newState", newState).d("error", error));

    bool registered = false;

    switch (newState) {
        case AuthObserverInterface::State::REFRESHED:
            if (AuthObserverInterface::Error::SUCCESS == error) {
                registered = true;
            }
            break;
        case AuthObserverInterface::State::UNINITIALIZED:
        case AuthObserverInterface::State::EXPIRED:
            registered = false;
            break;
        default:
            break;
    }

    m_executor.execute([this, registered]() {
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::REGISTRATION_STATUS] = toString(registered);
    });
}

void DevicePropertyAggregator::initializeAsyncPropertyMap() {
    m_asyncPropertyMap[DevicePropertyAggregatorInterface::TTS_PLAYER_STATE] = IDLE;
    m_asyncPropertyMap[DevicePropertyAggregatorInterface::AUDIO_PLAYER_STATE] = IDLE;
    m_asyncPropertyMap[DevicePropertyAggregatorInterface::CONTENT_ID] = NONE;
    m_asyncPropertyMap[DevicePropertyAggregatorInterface::ALERT_TYPE_AND_STATE] = IDLE;
    m_asyncPropertyMap[DevicePropertyAggregatorInterface::REGISTRATION_STATUS] = toString(false);

    auto contextOptional = getDeviceContextJson();
    if (contextOptional.hasValue()) {
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::DEVICE_CONTEXT] = contextOptional.value();
    }
}

std::unordered_map<std::string, std::string> DevicePropertyAggregator::getAllDeviceProperties() {
    auto future = m_executor.submit([this]() {
        auto allPropertyMap = getSyncDeviceProperties();

        /// get device context.
        auto deviceContext = getDeviceContextJson();
        if (deviceContext.hasValue()) {
            allPropertyMap[DEVICE_CONTEXT] = deviceContext.value();
        }

        allPropertyMap.insert(m_asyncPropertyMap.begin(), m_asyncPropertyMap.end());
        return allPropertyMap;
    });

    return future.get();
}
std::unordered_map<std::string, std::string> DevicePropertyAggregator::getSyncDeviceProperties() {
    std::unordered_map<std::string, std::string> syncDeviceProperties;
    for (auto property : listOfSynchronousProperties) {
        auto settingValue = getDeviceSetting(property);
        if (settingValue.hasValue()) {
            syncDeviceProperties[property] = settingValue.value();
        }
    }
    return syncDeviceProperties;
}
Optional<std::string> DevicePropertyAggregator::getDeviceProperty(const std::string& propertyKey) {
    auto maybePropertyValueFuture = m_executor.submit([this, propertyKey]() {
        Optional<std::string> maybePropertyValue;

        if (DevicePropertyAggregatorInterface::DEVICE_CONTEXT == propertyKey) {
            maybePropertyValue = getDeviceContextJson();
        } else if (listOfSynchronousProperties.find(propertyKey) != listOfSynchronousProperties.end()) {
            maybePropertyValue = getDeviceSetting(propertyKey);
        } else {
            auto it = m_asyncPropertyMap.find(propertyKey);
            if (it != m_asyncPropertyMap.end()) {
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

Optional<std::string> DevicePropertyAggregator::getDeviceSetting(const std::string& propertyKey) {
    Optional<std::string> maybePropertyValue;
    if (!m_deviceSettingsManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "Device Setting Manager is null"));
        return maybePropertyValue;
    }
    if (DevicePropertyAggregator::DO_NOT_DISTURB == propertyKey) {
        auto doNotDisturbSetting =
            m_deviceSettingsManager->getValue<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(false);

        if (doNotDisturbSetting.first) {
            std::stringstream ss;
            ss << std::boolalpha << doNotDisturbSetting.second;
            maybePropertyValue = ss.str();
        }
    } else if (DevicePropertyAggregator::LOCALE == propertyKey) {
        auto localeSetting = m_deviceSettingsManager->getValue<settings::DeviceSettingsIndex::LOCALE>();
        if (localeSetting.first) {
            auto localeSettingString = settings::toSettingString<settings::DeviceLocales>(localeSetting.second).second;
            maybePropertyValue = localeSettingString;
        }
    } else if (DevicePropertyAggregator::WAKE_WORDS == propertyKey) {
        auto wakeWordsSetting = m_deviceSettingsManager->getValue<settings::DeviceSettingsIndex::WAKE_WORDS>();
        if (wakeWordsSetting.first) {
            auto wakeWordSettingString = settings::toSettingString<settings::WakeWords>(wakeWordsSetting.second).second;
            maybePropertyValue = wakeWordSettingString;
        }
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "no matching parameter"));
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

void DevicePropertyAggregator::onAlertStateChange(const AlertObserverInterface::AlertInfo& alertInfo) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, alertInfo]() {
        std::stringstream ss;
        ss << alertInfo.type << ":" << alertInfo.state;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::ALERT_TYPE_AND_STATE] = ss.str();
    });
}

void DevicePropertyAggregator::onPlayerActivityChanged(
    PlayerActivity state,
    const AudioPlayerObserverInterface::Context& context) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, state, context]() {
        std::string playerActivityState = avsCommon::avs::playerActivityToString(state);
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::AUDIO_PLAYER_STATE] = playerActivityState;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::CONTENT_ID] = context.audioItemId;
    });
}

void DevicePropertyAggregator::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, status]() {
        std::stringstream ss;
        ss << status;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::CONNECTION_STATE] = ss.str();
    });
}

void DevicePropertyAggregator::onSetIndicator(IndicatorState state) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, state]() {
        std::stringstream ss;
        ss << state;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::NOTIFICATION_INDICATOR] = ss.str();
    });
}

void DevicePropertyAggregator::onNotificationReceived() {
    // No-op.
}

void DevicePropertyAggregator::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, newState]() {
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::TTS_PLAYER_STATE] =
            DialogUXStateObserverInterface::stateToString(newState);
    });
}

void DevicePropertyAggregator::onSpeakerSettingsChanged(
    const SpeakerManagerObserverInterface::Source& source,
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.execute([this, type, settings]() { updateSpeakerSettingsInPropertyMap(type, settings); });
}

void DevicePropertyAggregator::updateSpeakerSettingsInPropertyMap(
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__));
    std::string volumeAsString = std::to_string(settings.volume);

    std::string isMuteAsString = toString(settings.mute);

    switch (type) {
        case ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME:
            m_asyncPropertyMap[DevicePropertyAggregatorInterface::AVS_SPEAKER_VOLUME] = volumeAsString;
            m_asyncPropertyMap[DevicePropertyAggregatorInterface::AVS_SPEAKER_MUTE] = isMuteAsString;
            break;
        case ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME:
            m_asyncPropertyMap[DevicePropertyAggregatorInterface::AVS_ALERTS_VOLUME] = volumeAsString;
            m_asyncPropertyMap[DevicePropertyAggregatorInterface::AVS_ALERTS_MUTE] = isMuteAsString;
            break;
    }
}

void DevicePropertyAggregator::onRangeChanged(const RangeState& rangeState, const AlexaStateChangeCauseType cause) {
    ACSDK_DEBUG5(LX(__func__).d("range value", rangeState.value));
    m_executor.execute([this, rangeState]() {
        std::stringstream ss;
        ss << rangeState.value;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::RANGE_CONTROLLER_STATUS] = ss.str();
    });
}

void DevicePropertyAggregator::onPowerStateChanged(
    const PowerState& powerState,
    const AlexaStateChangeCauseType cause) {
    ACSDK_DEBUG5(LX(__func__).d("power state", powerState.powerState));
    m_executor.execute([this, powerState]() {
        std::stringstream ss;
        ss << powerState.powerState;
        m_asyncPropertyMap[DevicePropertyAggregatorInterface::POWER_CONTROLLER_STATUS] = ss.str();
    });
}

}  // namespace diagnostics
}  // namespace alexaClientSDK
