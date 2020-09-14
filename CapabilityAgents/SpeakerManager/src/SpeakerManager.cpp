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

#include <chrono>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "SpeakerManager/SpeakerManagerConstants.h"
#include "SpeakerManager/SpeakerManager.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::metrics;
using namespace rapidjson;

/// Speaker capability constants
/// Speaker interface type
static const std::string SPEAKER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// Speaker interface name
static const std::string SPEAKER_CAPABILITY_INTERFACE_NAME = "Speaker";
/// Speaker interface version
static const std::string SPEAKER_CAPABILITY_INTERFACE_VERSION = "1.0";
/// Retry timeout table
static const std::vector<int> DEFAULT_RETRY_TABLE = {std::chrono::milliseconds(10).count(),
                                                     std::chrono::milliseconds(20).count(),
                                                     std::chrono::milliseconds(40).count()};

/// String to identify log entries originating from this file.
static const std::string TAG{"SpeakerManager"};
/// The key in our config file to find the root of speaker manager configuration.
static const std::string SPEAKERMANAGER_CONFIGURATION_ROOT_KEY = "speakerManagerCapabilityAgent";
/// The key in our config file to find the minUnmuteVolume value.
static const std::string SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY = "minUnmuteVolume";

/// prefix for metrics emitted from the SpeakerManager CA
static const std::string SPEAKER_MANAGER_METRIC_PREFIX = "SPEAKER_MANAGER-";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Checks whether a value is within the bounds.
 *
 * @param value The value to check.
 * @param min The minimum value.
 * @param max The maximum value.
 */
template <class T>
static bool withinBounds(T value, T min, T max) {
    if (value < min || value > max) {
        ACSDK_ERROR(LX("checkBoundsFailed").d("value", value).d("min", min).d("max", max));
        return false;
    }
    return true;
}

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param eventName The name of the Metric event.
 * @param count Count of the metric that is emitted.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& eventName,
    int count) {
    auto metricEventBuilder = MetricEventBuilder{}
                                  .setActivityName(SPEAKER_MANAGER_METRIC_PREFIX + eventName)
                                  .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(count).build());

    auto metricEvent = metricEventBuilder.build();

    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

/**
 * Creates the Speaker capability configuration.
 *
 * @return The Speaker capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeakerCapabilityConfiguration();

std::shared_ptr<SpeakerManager> SpeakerManager::create(
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& groupVolumeInterfaces,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    } else if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    } else if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }

    int minUnmuteVolume = MIN_UNMUTE_VOLUME;

    auto configurationRoot = ConfigurationNode::getRoot()[SPEAKERMANAGER_CONFIGURATION_ROOT_KEY];
    // If key is present, then read and initilize the value from config or set to default.
    configurationRoot.getInt(SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY, &minUnmuteVolume, MIN_UNMUTE_VOLUME);

    auto speakerManager = std::shared_ptr<SpeakerManager>(new SpeakerManager(
        groupVolumeInterfaces,
        contextManager,
        messageSender,
        exceptionEncounteredSender,
        minUnmuteVolume,
        metricRecorder));

    return speakerManager;
}

SpeakerManager::SpeakerManager(
    const std::vector<std::shared_ptr<ChannelVolumeInterface>>& groupVolumeInterfaces,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    const int minUnmuteVolume,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"SpeakerManager"},
        m_metricRecorder{metricRecorder},
        m_contextManager{contextManager},
        m_messageSender{messageSender},
        m_minUnmuteVolume{minUnmuteVolume},
        m_retryTimer{DEFAULT_RETRY_TABLE},
        m_maxRetries{DEFAULT_RETRY_TABLE.size()},
        m_maximumVolumeLimit{AVS_SET_VOLUME_MAX} {
    for (auto groupVolume : groupVolumeInterfaces) {
        m_speakerMap.insert(std::pair<ChannelVolumeInterface::Type, std::shared_ptr<ChannelVolumeInterface>>(
            groupVolume->getSpeakerType(), groupVolume));
    }

    ACSDK_INFO(LX("mapCreated")
                   .d("numSpeakerVolume", m_speakerMap.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME))
                   .d("numAlertsVolume", m_speakerMap.count(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME)));

    // If we have at least one AVS_SPEAKER_VOLUME speaker, update the Context initially.
    const auto type = ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME;
    if (m_speakerMap.count(type)) {
        SpeakerInterface::SpeakerSettings settings;
        if (!validateSpeakerSettingsConsistency(type, &settings) || !updateContextManager(type, settings)) {
            ACSDK_ERROR(LX("initialUpdateContextManagerFailed"));
        }
    }

    m_capabilityConfigurations.insert(getSpeakerCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getSpeakerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SPEAKER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SPEAKER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SPEAKER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

DirectiveHandlerConfiguration SpeakerManager::getConfiguration() const {
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    DirectiveHandlerConfiguration configuration;
    configuration[SET_VOLUME] = audioNonBlockingPolicy;
    configuration[ADJUST_VOLUME] = audioNonBlockingPolicy;
    configuration[SET_MUTE] = audioNonBlockingPolicy;
    return configuration;
}

void SpeakerManager::doShutdown() {
    m_waitCancelEvent.wakeUp();
    m_executor.shutdown();
    m_messageSender.reset();
    m_contextManager.reset();
    m_observers.clear();
    m_speakerMap.clear();
}

void SpeakerManager::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op.
}

void SpeakerManager::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
};

bool SpeakerManager::parseDirectivePayload(std::string payload, Document* document) {
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

void SpeakerManager::sendExceptionEncountered(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const std::string& message,
    avsCommon::avs::ExceptionErrorType type) {
    m_executor.submit([this, info, message, type] {
        m_exceptionEncounteredSender->sendExceptionEncountered(info->directive->getUnparsedDirective(), type, message);
        if (info && info->result) {
            info->result->setFailed(message);
        }
        removeDirective(info);
    });
}

void SpeakerManager::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void SpeakerManager::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
        m_waitCancelEvent.wakeUp();
    }
}

void SpeakerManager::executeSendSpeakerSettingsChangedEvent(
    const std::string& eventName,
    SpeakerInterface::SpeakerSettings settings) {
    ACSDK_DEBUG9(LX("executeSendSpeakerSettingsChangedEvent"));
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(VOLUME_KEY, settings.volume, payload.GetAllocator());
    payload.AddMember(MUTED_KEY, settings.mute, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("executeSendSpeakerSettingsChangedEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString(eventName, "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

void SpeakerManager::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    const std::string directiveName = info->directive->getName();

    // Handling only AVS Speaker API volume here.
    ChannelVolumeInterface::Type directiveType = ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME;

    Document payload(kObjectType);
    if (!parseDirectivePayload(info->directive->getPayload(), &payload)) {
        sendExceptionEncountered(info, "Payload Parsing Failed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return;
    }

    /*
     * For AdjustVolume and SetVolume, unmute the speaker before a volume change. This behavior
     * is expected from a user perspective in many devices, and is aligned with 1P device behavior.
     */
    if (directiveName == SET_VOLUME.name) {
        int64_t volume;
        if (jsonUtils::retrieveValue(payload, VOLUME_KEY, &volume) &&
            withinBounds(volume, static_cast<int64_t>(AVS_SET_VOLUME_MIN), static_cast<int64_t>(AVS_SET_VOLUME_MAX))) {
            m_executor.submit([this, volume, directiveType, info] {
                /*
                 * Since AVS doesn't have a concept of Speaker IDs or types, no-op if a directive
                 * comes in and there are no AVS_SPEAKER_VOLUME speakers.
                 */
                if (m_speakerMap.count(directiveType) == 0) {
                    ACSDK_INFO(LX("noSpeakersRegistered").d("type", directiveType).m("swallowingDirective"));
                    executeSetHandlingCompleted(info);
                    return;
                }

                // Unmute before volume change.
                // Do not send a MuteChanged Event.
                if (executeSetMute(
                        directiveType,
                        false,
                        SpeakerManagerInterface::NotificationProperties(
                            SpeakerManagerObserverInterface::Source::DIRECTIVE, false, false)) &&
                    executeSetVolume(
                        directiveType,
                        static_cast<int8_t>(volume),
                        SpeakerManagerInterface::NotificationProperties(
                            SpeakerManagerObserverInterface::Source::DIRECTIVE))) {
                    executeSetHandlingCompleted(info);
                } else {
                    sendExceptionEncountered(info, "SetVolumeFailed", ExceptionErrorType::INTERNAL_ERROR);
                }
            });
        } else {
            sendExceptionEncountered(
                info, "Parsing Valid Payload Value Failed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        }
        // SET_VOLUME
    } else if (directiveName == ADJUST_VOLUME.name) {
        int64_t delta;
        if (jsonUtils::retrieveValue(payload, VOLUME_KEY, &delta) &&
            withinBounds(
                delta, static_cast<int64_t>(AVS_ADJUST_VOLUME_MIN), static_cast<int64_t>(AVS_ADJUST_VOLUME_MAX))) {
            m_executor.submit([this, delta, directiveType, info] {
                /*
                 * Since AVS doesn't have a concept of Speaker IDs or types, no-op if a directive
                 * comes in and there are no AVS_SPEAKER_VOLUME speakers.
                 */
                if (m_speakerMap.count(directiveType) == 0) {
                    ACSDK_INFO(LX("noSpeakersRegistered").d("type", directiveType).m("swallowingDirective"));
                    executeSetHandlingCompleted(info);
                    return;
                }

                // Unmute before volume change.
                // Do not send a MuteChanged Event.
                if (executeSetMute(
                        directiveType,
                        false,
                        SpeakerManagerInterface::NotificationProperties(
                            SpeakerManagerObserverInterface::Source::DIRECTIVE, false, false)) &&
                    executeAdjustVolume(
                        directiveType,
                        static_cast<int8_t>(delta),
                        SpeakerManagerInterface::NotificationProperties(
                            SpeakerManagerObserverInterface::Source::DIRECTIVE))) {
                    executeSetHandlingCompleted(info);
                } else {
                    sendExceptionEncountered(info, "SetVolumeFailed", ExceptionErrorType::INTERNAL_ERROR);
                }
            });
        } else {
            sendExceptionEncountered(
                info, "Parsing Valid Payload Value Failed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        }
        // ADJUST_VOLUME
    } else if (directiveName == SET_MUTE.name) {
        bool mute = false;
        if (jsonUtils::retrieveValue(payload, MUTE_KEY, &mute)) {
            m_executor.submit([this, mute, directiveType, info] {
                /*
                 * Since AVS doesn't have a concept of Speaker IDs or types, no-op if a directive
                 * comes in and there are no AVS_SPEAKER_VOLUME speakers.
                 */
                if (m_speakerMap.count(directiveType) == 0) {
                    ACSDK_INFO(LX("noSpeakersRegistered").d("type", directiveType).m("swallowingDirective"));
                    executeSetHandlingCompleted(info);
                    return;
                }

                bool success = true;
                if (!mute) {
                    success = executeRestoreVolume(directiveType, SpeakerManagerObserverInterface::Source::DIRECTIVE);
                }
                success &= executeSetMute(
                    directiveType,
                    mute,
                    SpeakerManagerInterface::NotificationProperties(
                        SpeakerManagerObserverInterface::Source::DIRECTIVE));

                if (success) {
                    executeSetHandlingCompleted(info);
                } else {
                    sendExceptionEncountered(info, "SetMuteFailed", ExceptionErrorType::INTERNAL_ERROR);
                }
            });
        } else {
            sendExceptionEncountered(
                info, "Parsing Valid Payload Value Failed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        }
        // SET_MUTE
    } else {
        sendExceptionEncountered(info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    }
}

void SpeakerManager::cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    removeDirective(info);
}

void SpeakerManager::addSpeakerManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("addSpeakerManagerObserverCalled"));
    if (!observer) {
        ACSDK_ERROR(LX("addSpeakerManagerObserverFailed").d("reason", "nullObserver"));
        return;
    }
    ACSDK_DEBUG9(LX("addSpeakerManagerObserver").d("observer", observer.get()));
    m_executor.submit([this, observer] {
        if (!m_observers.insert(observer).second) {
            ACSDK_ERROR(LX("addSpeakerManagerObserverFailed").d("reason", "duplicateObserver"));
        }
    });
}

void SpeakerManager::removeSpeakerManagerObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) {
    ACSDK_DEBUG9(LX("removeSpeakerManagerObserverCalled"));
    if (!observer) {
        ACSDK_ERROR(LX("removeSpeakerManagerObserverFailed").d("reason", "nullObserver"));
        return;
    }
    ACSDK_DEBUG9(LX("removeSpeakerManagerObserver").d("observer", observer.get()));
    m_executor.submit([this, observer] {
        if (m_observers.erase(observer) == 0) {
            ACSDK_WARN(LX("removeSpeakerManagerObserverFailed").d("reason", "nonExistentObserver"));
        }
    });
}

bool SpeakerManager::validateSpeakerSettingsConsistency(
    ChannelVolumeInterface::Type type,
    SpeakerInterface::SpeakerSettings* settings) {
    // Iterate through speakers and ensure all have the same volume.
    // Returns a pair of iterators.
    auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
    const auto begin = beginIteratorAndEndIterator.first;
    const auto end = beginIteratorAndEndIterator.second;
    bool consistent = true;

    SpeakerInterface::SpeakerSettings comparator;

    if (begin == end) {
        ACSDK_ERROR(
            LX("validateSpeakerSettingsConsistencyFailed").d("reason", "noSpeakersWithTypeFound").d("type", type));
        submitMetric(m_metricRecorder, "noSpeakersWithType", 1);
        return false;
    }
    submitMetric(m_metricRecorder, "noSpeakersWithType", 0);

    // Get settings value to compare the rest against.
    if (!begin->second->getSpeakerSettings(&comparator)) {
        ACSDK_ERROR(LX("validateSpeakerSettingsConsistencyFailed").d("reason", "gettingSpeakerSettingsFailed"));
        submitMetric(m_metricRecorder, "speakersCannotGetSetting", 1);
        return false;
    }

    std::multimap<ChannelVolumeInterface::Type, std::shared_ptr<ChannelVolumeInterface>>::iterator
        typeAndSpeakerIterator = begin;
    while (++typeAndSpeakerIterator != end) {
        auto volumeGroup = typeAndSpeakerIterator->second;
        SpeakerInterface::SpeakerSettings temp;

        // Retrieve speaker settings of current speaker.
        if (!volumeGroup->getSpeakerSettings(&temp)) {
            ACSDK_ERROR(LX("validateSpeakerSettingsConsistencyFailed").d("reason", "gettingSpeakerSettingsFailed"));
            submitMetric(m_metricRecorder, "speakersCannotGetSetting", 1);
            return false;
        }

        if (comparator.volume != temp.volume || comparator.mute != temp.mute) {
            submitMetric(m_metricRecorder, "speakersInconsistent", 1);
            ACSDK_ERROR(LX("validateSpeakerSettingsConsistencyFailed")
                            .d("reason", "inconsistentSpeakerSettings")
                            .d("comparatorVolume", static_cast<int>(comparator.volume))
                            .d("comparatorMute", comparator.mute)
                            .d("volume", static_cast<int>(temp.volume))
                            .d("mute", temp.mute));
            consistent = false;
            break;
        }
    }

    ACSDK_DEBUG9(LX("validateSpeakerSettingsConsistencyResult").d("consistent", consistent));
    submitMetric(m_metricRecorder, "speakersCannotGetSetting", 0);

    if (consistent && settings) {
        submitMetric(m_metricRecorder, "speakersInconsistent", 0);
        settings->volume = comparator.volume;
        settings->mute = comparator.mute;
        ACSDK_DEBUG9(
            LX("validateSpeakerSettings").d("volume", static_cast<int>(settings->volume)).d("mute", settings->mute));
    }

    return consistent;
}

bool SpeakerManager::updateContextManager(
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG9(LX("updateContextManagerCalled").d("speakerType", type));

    if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME != type) {
        ACSDK_DEBUG9(LX("updateContextManagerSkipped")
                         .d("reason", "typeMismatch")
                         .d("expected", ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)
                         .d("actual", type));
        return false;
    }

    rapidjson::Document state(rapidjson::kObjectType);
    state.AddMember(VOLUME_KEY, settings.volume, state.GetAllocator());
    state.AddMember(MUTED_KEY, settings.mute, state.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("updateContextManagerFailed").d("reason", "writeToBufferFailed"));
        return false;
    }

    if (SetStateResult::SUCCESS !=
        m_contextManager->setState(VOLUME_STATE, buffer.GetString(), StateRefreshPolicy::NEVER)) {
        ACSDK_ERROR(LX("updateContextManagerFailed").d("reason", "contextManagerSetStateFailed"));
        return false;
    }

    return true;
}

std::future<bool> SpeakerManager::setVolume(
    ChannelVolumeInterface::Type type,
    int8_t volume,
    const SpeakerManagerInterface::NotificationProperties& properties) {
    ACSDK_DEBUG9(LX(__func__).d("volume", static_cast<int>(volume)));
    return m_executor.submit([this, type, volume, properties] {
        return withinBounds(volume, AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX) &&
               executeSetVolume(type, volume, properties);
    });
}

std::future<bool> SpeakerManager::setVolume(
    ChannelVolumeInterface::Type type,
    int8_t volume,
    bool forceNoNotifications,
    SpeakerManagerObserverInterface::Source source) {
    ACSDK_DEBUG9(LX("setVolumeCalled").d("volume", static_cast<int>(volume)));
    SpeakerManagerInterface::NotificationProperties properties;
    properties.source = source;

    if (forceNoNotifications) {
        properties.notifyObservers = false;
        properties.notifyAVS = false;
    }
    return setVolume(type, volume, properties);
}

bool SpeakerManager::executeSetVolume(
    ChannelVolumeInterface::Type type,
    int8_t volume,
    const SpeakerManagerInterface::NotificationProperties& properties) {
    ACSDK_DEBUG9(LX("executeSetVolumeCalled").d("volume", static_cast<int>(volume)));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        submitMetric(m_metricRecorder, "setVolumeFailedZeroSpeakers", 1);
        return false;
    }
    submitMetric(m_metricRecorder, "setVolumeFailedZeroSpeakers", 0);
    submitMetric(m_metricRecorder, "setVolume", 1);
    if (volume == 0) {
        submitMetric(m_metricRecorder, "setVolumeZero", 1);
    }

    auto adjustedVolume = volume;

    auto maximumVolumeLimit = getMaximumVolumeLimit();

    if (volume > maximumVolumeLimit) {
        ACSDK_DEBUG0(LX("adjustingSetVolumeValue")
                         .d("reason", "valueHigherThanLimit")
                         .d("value", (int)volume)
                         .d("maximumVolumeLimitSetting", (int)maximumVolumeLimit));
        adjustedVolume = maximumVolumeLimit;
    }

    SpeakerInterface::SpeakerSettings settings;
    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "speakerSettingsInconsistent"));
        return false;
    }
    const int8_t previousVolume = settings.volume;

    retryAndApplySettings([this, type, adjustedVolume]() -> bool {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal
        // to type, and call setVolume.
        auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
        auto begin = beginIteratorAndEndIterator.first;
        auto end = beginIteratorAndEndIterator.second;

        for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
            auto speaker = typeAndSpeakerIterator->second;
            if (!speaker->setUnduckedVolume(adjustedVolume)) {
                submitMetric(m_metricRecorder, "setSpeakerVolumeFailed", 1);
                return false;
            }
        }

        submitMetric(m_metricRecorder, "setSpeakerVolumeFailed", 0);
        return true;
    });

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    updateContextManager(type, settings);

    if (properties.notifyObservers) {
        executeNotifyObserver(properties.source, type, settings);
    }

    if (properties.notifyAVS && !(previousVolume == settings.volume &&
                                  SpeakerManagerObserverInterface::Source::LOCAL_API == properties.source)) {
        executeNotifySettingsChanged(settings, VOLUME_CHANGED, properties.source, type);
    }

    return true;
}

bool SpeakerManager::executeRestoreVolume(
    ChannelVolumeInterface::Type type,
    SpeakerManagerObserverInterface::Source source) {
    SpeakerInterface::SpeakerSettings settings;

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "initialSpeakerSettingsInconsistent"));
        return false;
    }

    if (settings.volume > 0) {
        return true;
    }

    return executeSetVolume(type, m_minUnmuteVolume, SpeakerManagerInterface::NotificationProperties(source));
}

std::future<bool> SpeakerManager::adjustVolume(
    ChannelVolumeInterface::Type type,
    int8_t delta,
    const NotificationProperties& properties) {
    ACSDK_DEBUG9(LX(__func__).d("delta", static_cast<int>(delta)));
    return m_executor.submit([this, type, delta, properties] {
        return withinBounds(delta, AVS_ADJUST_VOLUME_MIN, getMaximumVolumeLimit()) &&
               executeAdjustVolume(type, delta, properties);
    });
}

std::future<bool> SpeakerManager::adjustVolume(
    ChannelVolumeInterface::Type type,
    int8_t delta,
    bool forceNoNotifications,
    SpeakerManagerObserverInterface::Source source) {
    ACSDK_DEBUG9(LX("adjustVolumeCalled").d("delta", static_cast<int>(delta)));
    SpeakerManagerInterface::NotificationProperties properties;
    properties.source = source;

    if (forceNoNotifications) {
        ACSDK_INFO(LX(__func__).d("reason", "forceNoNotifications").m("Skipping sending notifications"));
        properties.notifyObservers = false;
        properties.notifyAVS = false;
    }

    return adjustVolume(type, delta, properties);
}

bool SpeakerManager::executeAdjustVolume(
    ChannelVolumeInterface::Type type,
    int8_t delta,
    const SpeakerManagerInterface::NotificationProperties& properties) {
    ACSDK_DEBUG9(LX("executeAdjustVolumeCalled").d("delta", (int)delta));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }

    submitMetric(m_metricRecorder, "adjustVolume", 1);
    SpeakerInterface::SpeakerSettings settings;
    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    auto maxVolumeLimit = getMaximumVolumeLimit();
    // if the current volume settings is higher than the maxVolumelimit, reset it to maxVolumeLimit to apply delta to.
    if (settings.volume > maxVolumeLimit) {
        ACSDK_DEBUG0(LX("adjustingSettingsVolumeValue")
                         .d("reason", "valueHigherThanLimit")
                         .d("value", (int)settings.volume)
                         .d("maximumVolumeLimitSetting", (int)maxVolumeLimit));
        settings.volume = maxVolumeLimit;
    }

    const int8_t previousVolume = settings.volume;

    // calculate resultant volume
    if (delta > 0) {
        settings.volume = std::min((int)settings.volume + delta, (int)maxVolumeLimit);
    } else {
        settings.volume = std::max((int)settings.volume + delta, (int)AVS_SET_VOLUME_MIN);
    }

    retryAndApplySettings([this, type, settings] {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal
        // to type, and call setVolume.
        // NOTE : we dont use adjustVolume : since if a subset of speakers fail :
        // the delta will be reapplied
        auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
        auto begin = beginIteratorAndEndIterator.first;
        auto end = beginIteratorAndEndIterator.second;

        for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
            auto speaker = typeAndSpeakerIterator->second;
            if (!speaker->setUnduckedVolume(settings.volume)) {
                return false;
            }
        }
        return true;
    });

    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    ACSDK_DEBUG(LX("executeAdjustVolumeSuccess").d("newVolume", (int)settings.volume));

    updateContextManager(type, settings);

    if (properties.notifyObservers) {
        executeNotifyObserver(properties.source, type, settings);
    }

    if (properties.notifyAVS && !(previousVolume == settings.volume &&
                                  SpeakerManagerObserverInterface::Source::LOCAL_API == properties.source)) {
        executeNotifySettingsChanged(settings, VOLUME_CHANGED, properties.source, type);
    }

    return true;
}

std::future<bool> SpeakerManager::setMute(
    ChannelVolumeInterface::Type type,
    bool mute,
    const SpeakerManagerInterface::NotificationProperties& properties) {
    ACSDK_DEBUG9(LX(__func__).d("mute", mute));
    return m_executor.submit([this, type, mute, properties] { return executeSetMute(type, mute, properties); });
}

std::future<bool> SpeakerManager::setMute(
    ChannelVolumeInterface::Type type,
    bool mute,
    bool forceNoNotifications,
    SpeakerManagerObserverInterface::Source source) {
    ACSDK_DEBUG9(LX("setMuteCalled").d("mute", mute));
    SpeakerManagerInterface::NotificationProperties properties;
    properties.source = source;

    if (forceNoNotifications) {
        ACSDK_INFO(LX(__func__).d("reason", "forceNoNotifications").m("Skipping sending notifications"));
        properties.notifyObservers = false;
        properties.notifyAVS = false;
    }

    return setMute(type, mute, properties);
}

bool SpeakerManager::executeSetMute(
    ChannelVolumeInterface::Type type,
    bool mute,
    const SpeakerManagerInterface::NotificationProperties& properties) {
    ACSDK_DEBUG9(LX("executeSetMuteCalled").d("mute", mute));

    // if unmuting an already unmute speaker, then ignore the request
    if (!mute) {
        SpeakerInterface::SpeakerSettings settings;
        // All initialized speakers controlled by directives with the same type should have the same state.
        if (!validateSpeakerSettingsConsistency(type, &settings)) {
            ACSDK_WARN(LX("executeSetMuteWarn")
                           .m("cannot check if device is muted")
                           .d("reason", "speakerSettingsInconsistent"));
        } else if (!settings.mute) {
            return true;
        }
    }

    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeSetMuteFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }

    retryAndApplySettings([this, type, mute] {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal to type, and call setMute.
        auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
        auto begin = beginIteratorAndEndIterator.first;
        auto end = beginIteratorAndEndIterator.second;

        for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
            auto speaker = typeAndSpeakerIterator->second;
            if (!speaker->setMute(mute)) {
                return false;
            }
        }
        return true;
    });

    submitMetric(m_metricRecorder, mute ? "setMute" : "setUnMute", 1);
    SpeakerInterface::SpeakerSettings settings;

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeSetMute").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    updateContextManager(type, settings);

    if (properties.notifyObservers) {
        executeNotifyObserver(properties.source, type, settings);
    }

    if (properties.notifyAVS) {
        executeNotifySettingsChanged(settings, MUTE_CHANGED, properties.source, type);
    }

    return true;
}

#ifdef ENABLE_MAXVOLUME_SETTING
std::future<bool> SpeakerManager::setMaximumVolumeLimit(const int8_t maximumVolumeLimit) {
    return m_executor.submit([this, maximumVolumeLimit] {
        return withinBounds(maximumVolumeLimit, AVS_ADJUST_VOLUME_MIN, AVS_ADJUST_VOLUME_MAX) &&
               executeSetMaximumVolumeLimit(maximumVolumeLimit);
    });
}

bool SpeakerManager::executeSetMaximumVolumeLimit(const int8_t maximumVolumeLimit) {
    ACSDK_DEBUG3(LX(__func__).d("maximumVolumeLimit", static_cast<int>(maximumVolumeLimit)));

    // First adjust current volumes.
    for (auto it = m_speakerMap.begin(); it != m_speakerMap.end(); it = m_speakerMap.upper_bound(it->first)) {
        SpeakerInterface::SpeakerSettings speakerSettings;
        auto speakerType = it->first;
        ACSDK_DEBUG3(LX(__func__).d("type", speakerType));

        if (!executeGetSpeakerSettings(speakerType, &speakerSettings)) {
            ACSDK_ERROR(LX("executeSetMaximumVolumeLimitFailed").d("reason", "getSettingsFailed"));
            return false;
        }

        if (speakerSettings.volume > maximumVolumeLimit) {
            ACSDK_DEBUG1(LX("reducingVolume")
                             .d("reason", "volumeIsHigherThanNewLimit")
                             .d("type", it->first)
                             .d("volume", (int)speakerSettings.volume)
                             .d("limit", (int)maximumVolumeLimit));

            if (!executeSetVolume(
                    speakerType,
                    maximumVolumeLimit,
                    SpeakerManagerInterface::NotificationProperties(
                        SpeakerManagerObserverInterface::Source::DIRECTIVE))) {
                ACSDK_ERROR(LX("executeSetMaximumVolumeLimitFailed").d("reason", "setVolumeFailed"));
                return false;
            }
        }
    }
    m_maximumVolumeLimit = maximumVolumeLimit;
    return true;
}
#endif  // ENABLE_MAXVOLUME_SETTING

void SpeakerManager::executeNotifySettingsChanged(
    const SpeakerInterface::SpeakerSettings& settings,
    const std::string& eventName,
    const SpeakerManagerObserverInterface::Source& source,
    const ChannelVolumeInterface::Type& type) {
    // Only send an event if the AVS_SPEAKER_VOLUME settings changed.
    if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
        executeSendSpeakerSettingsChangedEvent(eventName, settings);
    } else {
        ACSDK_INFO(LX("eventNotSent").d("reason", "typeMismatch").d("speakerType", type));
    }
}

void SpeakerManager::executeNotifyObserver(
    const SpeakerManagerObserverInterface::Source& source,
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG9(LX("executeNotifyObserverCalled"));
    for (auto observer : m_observers) {
        observer->onSpeakerSettingsChanged(source, type, settings);
    }
}

std::future<bool> SpeakerManager::getSpeakerSettings(
    ChannelVolumeInterface::Type type,
    SpeakerInterface::SpeakerSettings* settings) {
    ACSDK_DEBUG9(LX("getSpeakerSettingsCalled"));
    return m_executor.submit([this, type, settings] { return executeGetSpeakerSettings(type, settings); });
}

bool SpeakerManager::executeGetSpeakerSettings(
    ChannelVolumeInterface::Type type,
    SpeakerInterface::SpeakerSettings* settings) {
    ACSDK_DEBUG9(LX("executeGetSpeakerSettingsCalled"));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeGetSpeakerSettingsFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }
    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, settings)) {
        ACSDK_ERROR(LX("executeGetSpeakerSettingsCalled").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    return true;
}

void SpeakerManager::addChannelVolumeInterface(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface) {
    if (!channelVolumeInterface) {
        ACSDK_ERROR(LX("addChannelVolumeInterfaceFailed").d("reason", "channelVolumeInterface cannot be nullptr"));
        return;
    }
    m_executor.submit([this, channelVolumeInterface] {
        m_speakerMap.insert(std::pair<ChannelVolumeInterface::Type, std::shared_ptr<ChannelVolumeInterface>>(
            channelVolumeInterface->getSpeakerType(), channelVolumeInterface));
    });
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SpeakerManager::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

int8_t SpeakerManager::getMaximumVolumeLimit() {
    return m_maximumVolumeLimit;
}

template <typename Task, typename... Args>
void SpeakerManager::retryAndApplySettings(Task task, Args&&... args) {
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);
    size_t attempt = 0;
    m_waitCancelEvent.reset();
    while (attempt < m_maxRetries) {
        if (boundTask()) {
            break;
        }

        // Exponential back-off before retry
        // Can be cancelled anytime
        if (m_waitCancelEvent.wait(m_retryTimer.calculateTimeToRetry(static_cast<int>(attempt)))) {
            break;
        }
        attempt++;
    }
}

}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
