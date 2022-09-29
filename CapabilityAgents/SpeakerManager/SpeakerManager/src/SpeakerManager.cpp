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
#include <sstream>

#include <acsdkManufactory/Annotated.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include <acsdk/SpeakerManager/private/SpeakerManagerConstants.h>
#include <acsdk/SpeakerManager/private/SpeakerManager.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerMiscStorage.h>

namespace alexaClientSDK {
namespace speakerManager {

using namespace acsdkManufactory;
using namespace acsdkShutdownManagerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::metrics;
using namespace rapidjson;

using DefaultEndpointAnnotation = avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation;

/// @addtogroup Lib_acsdkSpeakerManager
/// @{

/// Speaker capability constants
/// Speaker interface type
static const auto SPEAKER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// Speaker interface name
static const auto SPEAKER_CAPABILITY_INTERFACE_NAME = "Speaker";
/// Speaker interface version
static const auto SPEAKER_CAPABILITY_INTERFACE_VERSION = "1.0";
/// Retry timeout table
static const std::vector<int> DEFAULT_RETRY_TABLE = {static_cast<int>(std::chrono::milliseconds(10).count()),
                                                     static_cast<int>(std::chrono::milliseconds(20).count()),
                                                     static_cast<int>(std::chrono::milliseconds(40).count())};
/// prefix for metrics emitted from the SpeakerManager CA
static const auto SPEAKER_MANAGER_METRIC_PREFIX = "SPEAKER_MANAGER-";

/// String to identify log entries originating from this file.
#define TAG "SpeakerManager"
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Checks whether a value is within the bounds.
 *
 * @param value The value to check.
 * @param min The minimum value.
 * @param max The maximum value.
 * @private
 */
static bool withinBounds(std::int64_t value, std::int64_t min, std::int64_t max) noexcept {
    if (value < min || value > max) {
        ACSDK_ERROR(LX("checkBoundsFailed").d("value", value).d("min", min).d("max", max));
        return false;
    }
    return true;
}

/**
 * Converts the @c SpeakerManagerObserverInterface::Source to a string.
 *
 * @param source The input @c SpeakerManagerObserverInterface::Source.
 * @return The @c String representing the source.
 * @private
 */
static std::string getSourceString(SpeakerManagerObserverInterface::Source source) noexcept {
    std::stringstream ss;
    ss << source;
    return ss.str();
}

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param eventName The name of the Metric event.
 * @param count Count of the metric that is emitted.
 * @private
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& eventName,
    int count) noexcept {
    std::string activityName;
    activityName.reserve(sizeof(SPEAKER_MANAGER_METRIC_PREFIX) - 1 + eventName.size());
    activityName.append(SPEAKER_MANAGER_METRIC_PREFIX, sizeof(SPEAKER_MANAGER_METRIC_PREFIX) - 1).append(eventName);
    auto metricEventBuilder = MetricEventBuilder{}
                                  .setActivityName(activityName)
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
 * @private
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeakerCapabilityConfiguration() noexcept;

/// @}

std::shared_ptr<SpeakerManager> SpeakerManager::create(
    std::shared_ptr<SpeakerManagerConfigInterface> config,
    std::shared_ptr<SpeakerManagerStorageInterface> storage,
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& groupVolumeInterfaces,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) noexcept {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    } else if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    } else if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    } else if (!storage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStorage"));
        return nullptr;
    }

    return std::shared_ptr<SpeakerManager>(new SpeakerManager(
        std::move(config),
        std::move(storage),
        groupVolumeInterfaces,
        std::move(contextManager),
        std::move(messageSender),
        std::move(exceptionEncounteredSender),
        std::move(metricRecorder)));
}

SpeakerManager::SpeakerManager(
    std::shared_ptr<SpeakerManagerConfigInterface> speakerManagerConfig,
    std::shared_ptr<SpeakerManagerStorageInterface> speakerManagerStorage,
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& groupVolumeInterfaces,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) noexcept :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"SpeakerManager"},
        m_config{std::move(speakerManagerConfig), std::move(speakerManagerStorage)},
        m_metricRecorder{std::move(metricRecorder)},
        m_contextManager{std::move(contextManager)},
        m_messageSender{std::move(messageSender)},
        m_minUnmuteVolume{MIN_UNMUTE_VOLUME},
        m_retryTimer{DEFAULT_RETRY_TABLE},
        m_maxRetries{DEFAULT_RETRY_TABLE.size()},
        m_maximumVolumeLimit{AVS_SET_VOLUME_MAX},
        m_restoreMuteState{true} {
    for (auto& groupVolume : groupVolumeInterfaces) {
        addChannelVolumeInterfaceIntoSpeakerMap(groupVolume);
    }
    m_persistentStorage = m_config.getPersistentStorage();
    if (m_persistentStorage) {
        ACSDK_DEBUG5(LX("SpeakerManager").m("Persistent Storage is enabled."));
        loadConfiguration();      // Load configuration (either from storage, or from configuration).
        updateChannelSettings();  // Apply loaded configuration
    }
    m_capabilityConfigurations.insert(getSpeakerCapabilityConfiguration());
}

void SpeakerManager::addChannelVolumeInterfaceIntoSpeakerMap(
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface) noexcept {
    if (!channelVolumeInterface) {
        ACSDK_ERROR(LX("addChannelVolumeInterfaceFailed").d("reason", "nullChannelVolumeInterface"));
        return;
    }

    auto type = channelVolumeInterface->getSpeakerType();
    auto it = m_speakerMap.find(type);
    if (m_speakerMap.end() == it) {
        SpeakerSet speakerSet;
        speakerSet.insert(channelVolumeInterface);
        m_speakerMap.insert(std::make_pair(type, speakerSet));
        if (!executeInitializeSpeakerSettings(type)) {
            ACSDK_ERROR(LX("executeInitializeSpeakerSettings failed"));
        }
        // If we have one AVS_SPEAKER_VOLUME speaker, update the Context initially.
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            SpeakerInterface::SpeakerSettings settings;
            if (!executeGetSpeakerSettings(type, &settings) || !updateContextManager(type, settings)) {
                ACSDK_ERROR(LX(__func__).m("getSpeakerSettingsFailed or initialUpdateContextManagerFailed"));
            }
        }
    } else {
        if (it->second.find(channelVolumeInterface) != it->second.end()) {
            ACSDK_WARN(LX(__func__).d("type", type).m("Duplicated ChannelVolumeInterface"));
        } else {
            it->second.insert(channelVolumeInterface);
        }
    }

    ACSDK_DEBUG(LX(__func__).d("type", type).d("sizeOfSpeakerSet", m_speakerMap[type].size()));
}

std::shared_ptr<CapabilityConfiguration> getSpeakerCapabilityConfiguration() noexcept {
    std::unordered_map<std::string, std::string> configMap;
    // Create a type for gcc < 6.x
    using s = std::string;
    configMap.insert({s(CAPABILITY_INTERFACE_TYPE_KEY), s(SPEAKER_CAPABILITY_INTERFACE_TYPE)});
    configMap.insert({s(CAPABILITY_INTERFACE_NAME_KEY), s(SPEAKER_CAPABILITY_INTERFACE_NAME)});
    configMap.insert({s(CAPABILITY_INTERFACE_VERSION_KEY), s(SPEAKER_CAPABILITY_INTERFACE_VERSION)});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

DirectiveHandlerConfiguration SpeakerManager::getConfiguration() const {
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    DirectiveHandlerConfiguration configuration;
    configuration[SET_VOLUME] = neitherNonBlockingPolicy;
    configuration[ADJUST_VOLUME] = neitherNonBlockingPolicy;
    configuration[SET_MUTE] = neitherNonBlockingPolicy;
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

bool SpeakerManager::parseDirectivePayload(std::string payload, Document* document) noexcept {
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
    avsCommon::avs::ExceptionErrorType type) noexcept {
    m_executor.execute([this, info, message, type] {
        m_exceptionEncounteredSender->sendExceptionEncountered(info->directive->getUnparsedDirective(), type, message);
        if (info && info->result) {
            info->result->setFailed(message);
        }
        removeDirective(info);
    });
}

void SpeakerManager::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) noexcept {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void SpeakerManager::removeDirective(std::shared_ptr<DirectiveInfo> info) noexcept {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
        m_waitCancelEvent.wakeUp();
    }
}

void SpeakerManager::executeSendSpeakerSettingsChangedEvent(
    const std::string& eventName,
    SpeakerInterface::SpeakerSettings settings) noexcept {
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
            m_executor.execute([this, volume, directiveType, info] {
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
            m_executor.execute([this, delta, directiveType, info] {
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
            m_executor.execute([this, mute, directiveType, info] {
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
    m_executor.execute([this, observer] {
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
    m_executor.execute([this, observer] {
        if (m_observers.erase(observer) == 0) {
            ACSDK_WARN(LX("removeSpeakerManagerObserverFailed").d("reason", "nonExistentObserver"));
        }
    });
}

bool SpeakerManager::updateContextManager(
    const ChannelVolumeInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) noexcept {
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
    const SpeakerManagerInterface::NotificationProperties& properties) noexcept {
    ACSDK_DEBUG9(LX("executeSetVolumeCalled").d("volume", static_cast<int>(volume)));
    auto it = m_speakerMap.find(type);
    if (m_speakerMap.end() == it) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        submitMetric(m_metricRecorder, "setVolumeFailedZeroSpeakers", 1);
        return false;
    }
    submitMetric(m_metricRecorder, "setVolumeFailedZeroSpeakers", 0);
    submitMetric(m_metricRecorder, "setVolume", 1);
    if (volume == 0) {
        submitMetric(m_metricRecorder, "setVolumeZero", 1);
    }

    submitMetric(m_metricRecorder, "setVolumeSource_" + getSourceString(properties.source), 1);

    auto adjustedVolume = volume;

    auto maximumVolumeLimit = getMaximumVolumeLimit();

    if (volume > maximumVolumeLimit) {
        ACSDK_DEBUG0(LX("adjustingSetVolumeValue")
                         .d("reason", "valueHigherThanLimit")
                         .d("value", static_cast<int>(volume))
                         .d("maximumVolumeLimitSetting", static_cast<int>(maximumVolumeLimit)));
        adjustedVolume = maximumVolumeLimit;
    }

    SpeakerInterface::SpeakerSettings settings;
    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "executeGetSpeakerSettingsFailed"));
        return false;
    }
    const int8_t previousVolume = settings.volume;

    auto begin = it->second.begin();
    auto end = it->second.end();
    auto result = retryAndApplySettings([this, adjustedVolume, &begin, end]() -> bool {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal
        // to type, and call setVolume.
        while (begin != end) {
            if (!(*begin)->setUnduckedVolume(adjustedVolume)) {
                submitMetric(m_metricRecorder, "setSpeakerVolumeFailed", 1);
                return false;
            }
            begin++;
        }

        submitMetric(m_metricRecorder, "setSpeakerVolumeFailed", 0);
        return true;
    });

    if (!result) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "retryAndApplySettingsFailed"));
        return false;
    }

    settings.volume = adjustedVolume;
    if (!executeSetSpeakerSettings(type, settings)) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "executeSetSpeakerSettingsFailed"));
        return false;
    }

    ACSDK_DEBUG(LX("executeSetVolumeSuccess").d("newVolume", static_cast<int>(settings.volume)));

    if (m_persistentStorage) {
        if (previousVolume != settings.volume) {
            executePersistConfiguration();
        }
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

void SpeakerManager::convertSettingsToChannelState(
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
    SpeakerManagerStorageState::ChannelState* storageState) noexcept {
    const auto& settings = m_speakerSettings[type];
    storageState->channelVolume = settings.volume;
    storageState->channelMuteStatus = settings.mute;
}

void SpeakerManager::executePersistConfiguration() noexcept {
    SpeakerManagerStorageState state;
    convertSettingsToChannelState(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &state.speakerChannelState);
    convertSettingsToChannelState(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, &state.alertsChannelState);

    if (!m_config.saveState(state)) {
        ACSDK_ERROR(LX("executePersistConfigurationFailed"));
    } else {
        ACSDK_DEBUG(LX("executePersistConfigurationSuccess"));
    }
}

bool SpeakerManager::executeRestoreVolume(
    ChannelVolumeInterface::Type type,
    SpeakerManagerObserverInterface::Source source) noexcept {
    SpeakerInterface::SpeakerSettings settings;

    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeRestoreVolumeFailed").d("reason", "getSpeakerSettingsFailed"));
        return false;
    }

    if (settings.volume > 0) {
        return true;
    }

    return executeSetVolume(
        type, static_cast<int8_t>(m_minUnmuteVolume), SpeakerManagerInterface::NotificationProperties(source));
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
    const SpeakerManagerInterface::NotificationProperties& properties) noexcept {
    ACSDK_DEBUG9(LX("executeAdjustVolumeCalled").d("delta", static_cast<int>(delta)));
    auto it = m_speakerMap.find(type);
    if (m_speakerMap.end() == it) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }

    submitMetric(m_metricRecorder, "adjustVolume", 1);
    submitMetric(m_metricRecorder, "adjustVolumeSource_" + getSourceString(properties.source), 1);
    SpeakerInterface::SpeakerSettings settings;
    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "executeGetSpeakerSettingsFailed"));
        return false;
    }
    const int8_t previousVolume = settings.volume;

    auto maxVolumeLimit = getMaximumVolumeLimit();
    auto begin = it->second.begin();
    auto end = it->second.end();

    auto result = retryAndApplySettings([&begin, end, delta, maxVolumeLimit] {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal
        // to type, and call adjustUnduckedVolume.
        SpeakerInterface::SpeakerSettings speakerSettings;
        while (begin != end) {
            if (!(*begin)->getSpeakerSettings(&speakerSettings)) {
                return false;
            }
            // if the current volume settings is higher than the maxVolumelimit, reset it to maxVolumeLimit to apply
            // delta to.
            if (speakerSettings.volume > maxVolumeLimit) {
                ACSDK_DEBUG0(LX("adjustingSettingsVolumeValue")
                                 .d("reason", "valueHigherThanLimit")
                                 .d("value", (int)speakerSettings.volume)
                                 .d("maximumVolumeLimitSetting", (int)maxVolumeLimit));
                speakerSettings.volume = maxVolumeLimit;
            }
            // recalculate the delta if needed.
            int8_t newDelta = delta;
            if (delta > 0) {
                newDelta = static_cast<int8_t>(
                    std::min(
                        static_cast<int>(speakerSettings.volume) + static_cast<int>(delta),
                        static_cast<int>(maxVolumeLimit)) -
                    speakerSettings.volume);
            } else {
                newDelta = static_cast<int8_t>(
                    std::max(
                        static_cast<int>(speakerSettings.volume) + static_cast<int>(delta),
                        static_cast<int>(AVS_SET_VOLUME_MIN)) -
                    speakerSettings.volume);
            }

            if (!(*begin)->adjustUnduckedVolume(newDelta)) {
                return false;
            }
            begin++;
        }
        return true;
    });

    if (!result) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "retryAndApplySettingsFailed"));
        return false;
    }

    if (delta > 0) {
        settings.volume = static_cast<int8_t>(
            std::min(static_cast<int>(settings.volume) + static_cast<int>(delta), static_cast<int>(maxVolumeLimit)));
    } else {
        settings.volume = static_cast<int8_t>(std::max(
            static_cast<int>(settings.volume) + static_cast<int>(delta), static_cast<int>(AVS_SET_VOLUME_MIN)));
    }
    if (!executeSetSpeakerSettings(type, settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "executeSetSpeakerSettingsFailed"));
        return false;
    }

    ACSDK_DEBUG(LX("executeAdjustVolumeSuccess").d("newVolume", static_cast<int>(settings.volume)));

    if (m_persistentStorage) {
        if (previousVolume != settings.volume) {
            executePersistConfiguration();
        }
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
    const SpeakerManagerInterface::NotificationProperties& properties) noexcept {
    ACSDK_DEBUG9(LX("executeSetMuteCalled").d("mute", mute));
    SpeakerInterface::SpeakerSettings settings;
    if (!executeGetSpeakerSettings(type, &settings)) {
        ACSDK_ERROR(LX("executeSetMuteFailed").d("reason", "executeGetSpeakerSettingsFailed"));
        return false;
    }

    // if unmuting an already unmute speaker, then ignore the request
    if (!mute && !settings.mute) {
        ACSDK_DEBUG5(LX("executeSetMute").m("Device is already unmuted"));
        return true;
    }

    auto it = m_speakerMap.find(type);
    if (m_speakerMap.end() == it) {
        ACSDK_ERROR(LX("executeSetMuteFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }

    auto begin = it->second.begin();
    auto end = it->second.end();
    auto result = retryAndApplySettings([mute, &begin, end] {
        // Go through list of Speakers with ChannelVolumeInterface::Type equal to type, and call setMute.
        while (begin != end) {
            if (!(*begin)->setMute(mute)) {
                return false;
            }
            begin++;
        }
        return true;
    });

    if (!result) {
        ACSDK_ERROR(LX("executeSetMute").d("reason", "retryAndApplySettingsFailed"));
        return false;
    }

    submitMetric(m_metricRecorder, mute ? "setMute" : "setUnMute", 1);
    settings.mute = mute;
    if (mute) {
        submitMetric(m_metricRecorder, "setMuteSource_" + getSourceString(properties.source), 1);
    } else {
        submitMetric(m_metricRecorder, "setUnMuteSource_" + getSourceString(properties.source), 1);
    }
    if (!executeSetSpeakerSettings(type, settings)) {
        ACSDK_ERROR(LX("executeSetMuteFailed").d("reason", "executeSetSpeakerSettingsFailed"));
        return false;
    }

    ACSDK_DEBUG(LX("executeSetMuteSuccess").d("mute", mute));

    if (m_persistentStorage) {
        executePersistConfiguration();
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
std::future<bool> SpeakerManager::setMaximumVolumeLimit(const int8_t maximumVolumeLimit) noexcept {
    return m_executor.submit([this, maximumVolumeLimit] {
        return withinBounds(maximumVolumeLimit, AVS_ADJUST_VOLUME_MIN, AVS_ADJUST_VOLUME_MAX) &&
               executeSetMaximumVolumeLimit(maximumVolumeLimit);
    });
}

bool SpeakerManager::executeSetMaximumVolumeLimit(const int8_t maximumVolumeLimit) noexcept {
    ACSDK_DEBUG3(LX(__func__).d("maximumVolumeLimit", static_cast<int>(maximumVolumeLimit)));

    // First adjust current volumes.
    for (auto it = m_speakerMap.begin(); it != m_speakerMap.end(); it++) {
        auto speakerType = it->first;
        ACSDK_DEBUG3(LX(__func__).d("type", speakerType));
        SpeakerInterface::SpeakerSettings speakerSettings;
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
    const ChannelVolumeInterface::Type& type) noexcept {
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
    const SpeakerInterface::SpeakerSettings& settings) noexcept {
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
    SpeakerInterface::SpeakerSettings* settings) noexcept {
    ACSDK_DEBUG9(LX("executeGetSpeakerSettingsCalled"));
    if (!settings) {
        ACSDK_ERROR(LX("executeGetSpeakerSettingsFailed").d("reason", "nullSettings"));
        return false;
    }
    if (m_speakerMap.find(type) == m_speakerMap.end()) {
        ACSDK_ERROR(LX("executeGetSpeakerSettingsFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }
    // m_speakerSettings is the main source of truth, only query actual speaker as a fallback
    if (m_speakerSettings.find(type) == m_speakerSettings.end()) {
        ACSDK_WARN(LX("executeGetSpeakerSettings").m("noSpeakerSettingsWithType, initializing it").d("type", type));
        if (!executeInitializeSpeakerSettings(type)) {
            ACSDK_ERROR(LX("executeGetSpeakerSettingsFailed").d("reason", "initializeSpeakerSettingsFailed"));
            return false;
        }
    }
    *settings = m_speakerSettings[type];
    return true;
}

bool SpeakerManager::executeInitializeSpeakerSettings(
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type) noexcept {
    ACSDK_DEBUG5(LX("executeInitializeSpeakerSettings").d("type", type));
    auto it = m_speakerMap.find(type);
    if (m_speakerMap.end() == it) {
        ACSDK_ERROR(LX("executeInitializeSpeakerSettings").d("reason", "noSpeakersWithTypeFound").d("type", type));
        return false;
    }

    auto begin = it->second.begin();
    SpeakerInterface::SpeakerSettings settings;
    if (!(*begin)->getSpeakerSettings(&settings)) {
        ACSDK_ERROR(LX("executeInitializeSpeakerSettings").d("reason", "gettingSpeakerSettingsFailed").d("type", type));
        return false;
    }

    if (m_persistentStorage) {
        auto defaultSettingsIt = m_speakerSettings.find(type);
        if (m_speakerSettings.end() == defaultSettingsIt) {
            ACSDK_DEBUG5(LX("executeInitializeSpeakerSettings").d("Initializing new speaker setting", type));
            m_speakerSettings[type] = settings;
        }
    } else {
        m_speakerSettings[type] = settings;
    }

    return true;
}

void SpeakerManager::onExternalSpeakerSettingsUpdate(
    ChannelVolumeInterface::Type type,
    const SpeakerInterface::SpeakerSettings& speakerSettings,
    const NotificationProperties& properties) {
    ACSDK_DEBUG9(LX("onExternalSpeakerSettingsUpdate"));
    m_executor.execute([this, type, speakerSettings, properties] {
        auto it = m_speakerMap.find(type);
        if (m_speakerMap.end() == it) {
            ACSDK_ERROR(LX("onExternalSpeakerSettingsUpdateFailed").d("reason", "noSpeakersWithType").d("type", type));
            submitMetric(m_metricRecorder, "onExternalSpeakerSettingsUpdateFailedZeroSpeakers", 1);
            return;
        }

        submitMetric(m_metricRecorder, "onExternalSpeakerSettingsUpdateFailedZeroSpeakers", 0);
        submitMetric(m_metricRecorder, "onExternalSpeakerSettingsUpdate", 1);
        submitMetric(
            m_metricRecorder, "onExternalSpeakerSettingsUpdateSource_" + getSourceString(properties.source), 1);

        auto adjustedVolume =
            (speakerSettings.volume > AVS_SET_VOLUME_MIN) ? speakerSettings.volume : AVS_SET_VOLUME_MIN;

        auto maximumVolumeLimit = getMaximumVolumeLimit();

        if (speakerSettings.volume > maximumVolumeLimit) {
            ACSDK_DEBUG0(LX("adjustingUpdatedVolumeValue")
                             .d("reason", "valueHigherThanLimit")
                             .d("value", static_cast<int>(speakerSettings.volume))
                             .d("maximumVolumeLimitSetting", static_cast<int>(maximumVolumeLimit)));
            adjustedVolume = maximumVolumeLimit;
        }
        SpeakerInterface::SpeakerSettings settings;
        if (!executeGetSpeakerSettings(type, &settings)) {
            ACSDK_ERROR(LX("onExternalSpeakerSettingsUpdateFailed").d("reason", "executeGetSpeakerSettingsFailed"));
            return;
        }
        auto previousVolume = settings.volume;
        auto previousMute = settings.mute;

        // update the new settings.
        settings.volume = adjustedVolume;
        settings.mute = speakerSettings.mute;
        if (!executeSetSpeakerSettings(type, settings)) {
            ACSDK_ERROR(LX("executeOnVolumeUpdatedFailed").d("reason", "executeSetSpeakerSettingsFailed"));
            return;
        }

        if (m_persistentStorage) {
            if ((previousVolume != settings.volume) || (previousMute != settings.mute)) {
                executePersistConfiguration();
            }
        }

        updateContextManager(type, settings);

        if (properties.notifyObservers) {
            executeNotifyObserver(properties.source, type, settings);
        }

        if (properties.notifyAVS) {
            if (!(previousVolume == settings.volume &&
                  SpeakerManagerObserverInterface::Source::LOCAL_API == properties.source)) {
                executeNotifySettingsChanged(settings, VOLUME_CHANGED, properties.source, type);
            }
            if (previousMute != settings.mute) {
                executeNotifySettingsChanged(settings, MUTE_CHANGED, properties.source, type);
            }
        }
        return;
    });
}

bool SpeakerManager::executeSetSpeakerSettings(
    const ChannelVolumeInterface::Type type,
    const SpeakerInterface::SpeakerSettings& settings) noexcept {
    ACSDK_DEBUG9(LX("executeSetSpeakerSettingsCalled"));
    if (m_speakerMap.end() == m_speakerMap.find(type)) {
        ACSDK_ERROR(LX("executeSetSpeakerSettings").d("reason", "noSpeakersWithTypeFound").d("type", type));
        return false;
    }

    m_speakerSettings[type] = settings;
    return true;
}

void SpeakerManager::addChannelVolumeInterface(std::shared_ptr<ChannelVolumeInterface> channelVolumeInterface) {
    addChannelVolumeInterfaceIntoSpeakerMap(channelVolumeInterface);
    if (m_persistentStorage) {
        SpeakerInterface::SpeakerSettings& settings = m_speakerSettings[channelVolumeInterface->getSpeakerType()];
        retryAndApplySettings([this, &settings, &channelVolumeInterface]() -> bool {
            ACSDK_DEBUG9(LX(__func__)
                             .d("speaker id", channelVolumeInterface->getId())
                             .d("speaker type", channelVolumeInterface->getSpeakerType())
                             .d("default volume set to ", settings.volume));
            if (!channelVolumeInterface->setUnduckedVolume(settings.volume)) {
                submitMetric(m_metricRecorder, "setVolumeFailed", 1);
                return false;
            }
            if (!channelVolumeInterface->setMute(settings.mute)) {
                submitMetric(m_metricRecorder, "setMuteFailed", 1);
                return false;
            }
            submitMetric(m_metricRecorder, "setVolumeFailed", 0);
            submitMetric(m_metricRecorder, "setMuteFailed", 0);
            return true;
        });
    }
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SpeakerManager::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

int8_t SpeakerManager::getMaximumVolumeLimit() noexcept {
    return m_maximumVolumeLimit;
}

bool SpeakerManager::retryAndApplySettings(const std::function<bool()>& task) noexcept {
    size_t attempt = 0;
    m_waitCancelEvent.reset();
    while (attempt < m_maxRetries) {
        if (task()) {
            break;
        }

        // Exponential back-off before retry
        // Can be cancelled anytime
        if (m_waitCancelEvent.wait(m_retryTimer.calculateTimeToRetry(static_cast<int>(attempt)))) {
            break;
        }
        attempt++;
    }
    return attempt < m_maxRetries;
}

int8_t SpeakerManager::adjustVolumeRange(int64_t volume) noexcept {
    auto adjustedVolume = std::min(
        static_cast<int64_t>(AVS_ADJUST_VOLUME_MAX), std::max(static_cast<int64_t>(AVS_ADJUST_VOLUME_MIN), volume));
    return static_cast<int8_t>(adjustedVolume);
}

void SpeakerManager::presetChannelDefaults(
    ChannelVolumeInterface::Type type,
    const SpeakerManagerStorageState::ChannelState& state) noexcept {
    auto adjustedVolume = adjustVolumeRange(state.channelVolume);

    if (adjustedVolume != state.channelVolume) {
        ACSDK_DEBUG9(LX(__func__)
                         .m("adjusted configured value")
                         .d("type", type)
                         .d("configured volume", state.channelVolume)
                         .d("adjusted volume", adjustedVolume));
    }

    m_speakerSettings[type].volume = adjustedVolume;
    if (m_restoreMuteState) {
        m_speakerSettings[type].mute = state.channelMuteStatus;
    }
}

void SpeakerManager::loadConfiguration() noexcept {
    ACSDK_DEBUG5(LX("configureDefaults").m("Loading configuration"));

    m_minUnmuteVolume = m_config.getMinUnmuteVolume();
    m_restoreMuteState = m_config.getRestoreMuteState();

    SpeakerManagerStorageState state;
    m_config.loadState(state);

    presetChannelDefaults(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, state.speakerChannelState);
    presetChannelDefaults(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, state.alertsChannelState);
}

void SpeakerManager::updateChannelSettings() noexcept {
    updateChannelSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);
    updateChannelSettings(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME);
}

void SpeakerManager::updateChannelSettings(ChannelVolumeInterface::Type type) noexcept {
    auto it = m_speakerMap.find(type);
    if (it != m_speakerMap.end()) {
        SpeakerInterface::SpeakerSettings& settings = m_speakerSettings[type];

        auto begin = it->second.begin();
        auto end = it->second.end();
        retryAndApplySettings([this, &settings, &begin, end]() -> bool {
            // Go through list of Speakers with ChannelVolumeInterface::Type equal
            // to type, and call setVolume.
            while (begin != end) {
                ACSDK_DEBUG9(LX(__func__)
                                 .d("speaker id", (*begin)->getId())
                                 .d("speaker type", (*begin)->getSpeakerType())
                                 .d("default volume set to ", settings.volume));
                if (!(*begin)->setUnduckedVolume(settings.volume)) {
                    submitMetric(m_metricRecorder, "setVolumeFailed", 1);
                    return false;
                }
                if (!(*begin)->setMute(settings.mute)) {
                    submitMetric(m_metricRecorder, "setMuteFailed", 1);
                    return false;
                }
                begin++;
            }

            submitMetric(m_metricRecorder, "setVolumeFailed", 0);
            submitMetric(m_metricRecorder, "setMuteFailed", 0);
            return true;
        });

        executeInitializeSpeakerSettings(type);
    }
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
