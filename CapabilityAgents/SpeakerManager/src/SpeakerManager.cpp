/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "SpeakerManager/SpeakerManagerConstants.h"

#include "SpeakerManager/SpeakerManager.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// Speaker capability constants
/// Speaker interface type
static const std::string SPEAKER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// Speaker interface name
static const std::string SPEAKER_CAPABILITY_INTERFACE_NAME = "Speaker";
/// Speaker interface version
static const std::string SPEAKER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// String to identify log entries originating from this file.
static const std::string TAG{"SpeakerManager"};

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
 * Creates the Speaker capability configuration.
 *
 * @return The Speaker capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeakerCapabilityConfiguration();

std::shared_ptr<SpeakerManager> SpeakerManager::create(
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& speakers,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
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

    auto speakerManager = std::shared_ptr<SpeakerManager>(
        new SpeakerManager(speakers, contextManager, messageSender, exceptionEncounteredSender));

    return speakerManager;
}

SpeakerManager::SpeakerManager(
    const std::vector<std::shared_ptr<SpeakerInterface>>& speakers,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"SpeakerManager"},
        m_contextManager{contextManager},
        m_messageSender{messageSender} {
    for (auto speaker : speakers) {
        m_speakerMap.insert(
            std::pair<SpeakerInterface::Type, std::shared_ptr<SpeakerInterface>>(speaker->getSpeakerType(), speaker));
    }

    ACSDK_DEBUG(LX("mapCreated")
                    .d("numSpeakerVolume", m_speakerMap.count(SpeakerInterface::Type::AVS_SPEAKER_VOLUME))
                    .d("numAlertsVolume", m_speakerMap.count(SpeakerInterface::Type::AVS_ALERTS_VOLUME)));

    // If we have at least one AVS_SPEAKER_VOLUME speaker, update the Context initially.
    const auto type = SpeakerInterface::Type::AVS_SPEAKER_VOLUME;
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
    SpeakerInterface::Type directiveType = SpeakerInterface::Type::AVS_SPEAKER_VOLUME;

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
                if (executeSetMute(directiveType, false, SpeakerManagerObserverInterface::Source::DIRECTIVE, true) &&
                    executeSetVolume(
                        directiveType,
                        static_cast<int8_t>(volume),
                        SpeakerManagerObserverInterface::Source::DIRECTIVE)) {
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
                if (executeSetMute(directiveType, false, SpeakerManagerObserverInterface::Source::DIRECTIVE, true) &&
                    executeAdjustVolume(
                        directiveType,
                        static_cast<int8_t>(delta),
                        SpeakerManagerObserverInterface::Source::DIRECTIVE)) {
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

                if (executeSetMute(directiveType, mute, SpeakerManagerObserverInterface::Source::DIRECTIVE)) {
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
    SpeakerInterface::Type type,
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
        return false;
    }

    // Get settings value to compare the rest against.
    if (!begin->second->getSpeakerSettings(&comparator)) {
        ACSDK_ERROR(LX("validateSpeakerSettingsConsistencyFailed").d("reason", "gettingSpeakerSettingsFailed"));
        return false;
    }

    std::multimap<SpeakerInterface::Type, std::shared_ptr<SpeakerInterface>>::iterator typeAndSpeakerIterator = begin;
    while (++typeAndSpeakerIterator != end) {
        auto speaker = typeAndSpeakerIterator->second;
        SpeakerInterface::SpeakerSettings temp;

        // Retrieve speaker settings of current speaker.
        if (!speaker->getSpeakerSettings(&temp)) {
            ACSDK_ERROR(LX("validateSpeakerSettingsConsistencyFailed").d("reason", "gettingSpeakerSettingsFailed"));
            return false;
        }

        if (comparator.volume != temp.volume || comparator.mute != temp.mute) {
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

    if (consistent && settings) {
        settings->volume = comparator.volume;
        settings->mute = comparator.mute;
        ACSDK_DEBUG9(
            LX("validateSpeakerSettings").d("volume", static_cast<int>(settings->volume)).d("mute", settings->mute));
    }

    return consistent;
}

bool SpeakerManager::updateContextManager(
    const SpeakerInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG9(LX("updateContextManagerCalled").d("speakerType", type));

    if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME != type) {
        ACSDK_DEBUG(LX("updateContextManagerSkipped")
                        .d("reason", "typeMismatch")
                        .d("expected", SpeakerInterface::Type::AVS_SPEAKER_VOLUME)
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

std::future<bool> SpeakerManager::setVolume(SpeakerInterface::Type type, int8_t volume, bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("setVolumeCalled").d("volume", static_cast<int>(volume)));
    return m_executor.submit([this, type, volume, forceNoNotifications] {
        return withinBounds(volume, AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX) &&
               executeSetVolume(type, volume, SpeakerManagerObserverInterface::Source::LOCAL_API, forceNoNotifications);
    });
}

bool SpeakerManager::executeSetVolume(
    SpeakerInterface::Type type,
    int8_t volume,
    SpeakerManagerObserverInterface::Source source,
    bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("executeSetVolumeCalled").d("volume", static_cast<int>(volume)));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeSetVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }
    // Go through list of Speakers with SpeakerInterface::Type equal to type, and call setVolume.
    auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
    auto begin = beginIteratorAndEndIterator.first;
    auto end = beginIteratorAndEndIterator.second;

    for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
        auto speaker = typeAndSpeakerIterator->second;
        // In the future retry logic could be useful to ensure speakers are consistent.
        if (!speaker->setVolume(volume)) {
            return false;
        }
    }

    SpeakerInterface::SpeakerSettings settings;

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeSetVolume").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    updateContextManager(type, settings);

    if (forceNoNotifications) {
        ACSDK_INFO(LX("executeSetVolume").m("Skipping sending notifications").d("reason", "forceNoNotifications"));
    } else {
        executeNotifySettingsChanged(settings, VOLUME_CHANGED, source, type);
    }
    return true;
}

std::future<bool> SpeakerManager::adjustVolume(SpeakerInterface::Type type, int8_t delta, bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("adjustVolumeCalled").d("delta", static_cast<int>(delta)));
    return m_executor.submit([this, type, delta, forceNoNotifications] {
        return withinBounds(delta, AVS_ADJUST_VOLUME_MIN, AVS_ADJUST_VOLUME_MAX) &&
               executeAdjustVolume(
                   type, delta, SpeakerManagerObserverInterface::Source::LOCAL_API, forceNoNotifications);
    });
}

bool SpeakerManager::executeAdjustVolume(
    SpeakerInterface::Type type,
    int8_t delta,
    SpeakerManagerObserverInterface::Source source,
    bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("executeAdjustVolumeCalled").d("delta", (int)delta));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }
    SpeakerInterface::SpeakerSettings settings;

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "initialSpeakerSettingsInconsistent"));
        return false;
    }

    // Go through list of Speakers with SpeakerInterface::Type equal to type, and call adjustVolume.
    auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
    auto begin = beginIteratorAndEndIterator.first;
    auto end = beginIteratorAndEndIterator.second;

    for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
        auto speaker = typeAndSpeakerIterator->second;
        // In the future retry logic could be useful to ensure speakers are consistent.
        if (!speaker->adjustVolume(delta)) {
            return false;
        }
    }

    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeAdjustVolumeFailed").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    ACSDK_DEBUG(LX("executeAdjustVolumeSuccess").d("newVolume", (int)settings.volume));

    updateContextManager(type, settings);

    if (forceNoNotifications) {
        ACSDK_INFO(LX("executeAdjustVolume").m("Skipping sending notifications").d("reason", "forceNoNotifications"));
    } else {
        executeNotifySettingsChanged(settings, VOLUME_CHANGED, source, type);
    }

    return true;
}

std::future<bool> SpeakerManager::setMute(SpeakerInterface::Type type, bool mute, bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("setMuteCalled").d("mute", mute));
    return m_executor.submit([this, type, mute, forceNoNotifications] {
        return executeSetMute(type, mute, SpeakerManagerObserverInterface::Source::LOCAL_API, forceNoNotifications);
    });
}

bool SpeakerManager::executeSetMute(
    SpeakerInterface::Type type,
    bool mute,
    SpeakerManagerObserverInterface::Source source,
    bool forceNoNotifications) {
    ACSDK_DEBUG9(LX("executeSetMuteCalled").d("mute", mute));
    if (m_speakerMap.count(type) == 0) {
        ACSDK_ERROR(LX("executeSetMuteFailed").d("reason", "noSpeakersWithType").d("type", type));
        return false;
    }

    // Go through list of Speakers with SpeakerInterface::Type equal to type, and call setMute.
    auto beginIteratorAndEndIterator = m_speakerMap.equal_range(type);
    auto begin = beginIteratorAndEndIterator.first;
    auto end = beginIteratorAndEndIterator.second;

    for (auto typeAndSpeakerIterator = begin; typeAndSpeakerIterator != end; typeAndSpeakerIterator++) {
        auto speaker = typeAndSpeakerIterator->second;
        // In the future retry logic could be useful to ensure speakers are consistent.
        if (!speaker->setMute(mute)) {
            return false;
        }
    }

    SpeakerInterface::SpeakerSettings settings;

    // All initialized speakers controlled by directives with the same type should have the same state.
    if (!validateSpeakerSettingsConsistency(type, &settings)) {
        ACSDK_ERROR(LX("executeSetMute").d("reason", "speakerSettingsInconsistent"));
        return false;
    }

    updateContextManager(type, settings);

    if (forceNoNotifications) {
        ACSDK_INFO(LX("executeSetMute").m("Skipping sending notifications").d("reason", "forceNoNotifications"));
    } else {
        executeNotifySettingsChanged(settings, MUTE_CHANGED, source, type);
    }

    return true;
}

void SpeakerManager::executeNotifySettingsChanged(
    const SpeakerInterface::SpeakerSettings& settings,
    const std::string& eventName,
    const SpeakerManagerObserverInterface::Source& source,
    const SpeakerInterface::Type& type) {
    executeNotifyObserver(source, type, settings);

    // Only send an event if the AVS_SPEAKER_VOLUME settings changed.
    if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
        executeSendSpeakerSettingsChangedEvent(eventName, settings);
    } else {
        ACSDK_INFO(LX("eventNotSent").d("reason", "typeMismatch").d("speakerType", type));
    }
}

void SpeakerManager::executeNotifyObserver(
    const SpeakerManagerObserverInterface::Source& source,
    const SpeakerInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG9(LX("executeNotifyObserverCalled"));
    for (auto observer : m_observers) {
        observer->onSpeakerSettingsChanged(source, type, settings);
    }
}

std::future<bool> SpeakerManager::getSpeakerSettings(
    SpeakerInterface::Type type,
    SpeakerInterface::SpeakerSettings* settings) {
    ACSDK_DEBUG9(LX("getSpeakerSettingsCalled"));
    return m_executor.submit([this, type, settings] { return executeGetSpeakerSettings(type, settings); });
}

bool SpeakerManager::executeGetSpeakerSettings(
    SpeakerInterface::Type type,
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

void SpeakerManager::addSpeaker(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker) {
    if (!speaker) {
        ACSDK_ERROR(LX("addSpeakerFailed").d("reason", "speaker cannot be nullptr"));
        return;
    }
    m_speakerMap.insert(
        std::pair<SpeakerInterface::Type, std::shared_ptr<SpeakerInterface>>(speaker->getSpeakerType(), speaker));
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SpeakerManager::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
