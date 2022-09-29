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

#include <acsdk/SpeakerManager/private/SpeakerManager.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerMiscStorage.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace speakerManager {

using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::utils::json;

/// @addtogroup Lib_acsdkSpeakerManager
/// @{

/// String to identify log entries originating from this file.
/// @private
#define TAG "SpeakerManagerMiscStorage"

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Component name for Misc DB.
/// @private
static const auto COMPONENT_NAME = "SpeakerManager";
/// Misc DB table for component state.
/// @private
static const auto COMPONENT_STATE_TABLE = "SpeakerManagerConfig";
/// Misc DB table entry for component state.
/// @private
static const auto COMPONENT_STATE_KEY = "SpeakerManagerConfig";
/// The key in our config for speaker volume.
/// @private
static const auto SPEAKER_CHANNEL_STATE = "speakerChannelState";
/// The key in our config for speaker volume.
/// @private
static const auto ALERTS_CHANNEL_STATE = "alertsChannelState";
/// The key in our config for speaker volume.
/// @private
static const auto CHANNEL_VOLUME_KEY = "channelVolume";
/// The key in our config for speaker volume.
/// @private
static const auto CHANNEL_MUTE_STATUS_KEY = "channelMuteStatus";

/// @}

std::shared_ptr<SpeakerManagerMiscStorage> SpeakerManagerMiscStorage::create(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) noexcept {
    if (miscStorage) {
        auto res = std::shared_ptr<SpeakerManagerMiscStorage>(new SpeakerManagerMiscStorage(std::move(miscStorage)));
        if (res->init()) {
            return res;
        } else {
            ACSDK_ERROR(LX("createFailed").d("reason", "failedToInitialize"));
        }
    } else {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMiscStorage"));
    }
    return nullptr;
}

SpeakerManagerMiscStorage::SpeakerManagerMiscStorage(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) noexcept :
        m_miscStorage{std::move(miscStorage)} {
}

bool SpeakerManagerMiscStorage::init() noexcept {
    if (!m_miscStorage->isOpened() && !m_miscStorage->open()) {
        ACSDK_DEBUG3(LX(__func__).m("Couldn't open misc database. Creating."));
        if (!m_miscStorage->createDatabase()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not create misc database."));
            return false;
        }
    }

    bool tableExists = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, COMPONENT_STATE_TABLE, &tableExists)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not check state table information in misc database."));
        return false;
    }

    if (!tableExists) {
        ACSDK_DEBUG3(LX(__func__).m("Table doesn't exist in misc database. Creating new."));
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                COMPONENT_STATE_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "Cannot create table")
                            .d("table", COMPONENT_STATE_TABLE)
                            .d("key", COMPONENT_STATE_KEY)
                            .d("component", COMPONENT_NAME));
            return false;
        }
    }
    return true;
}

bool SpeakerManagerMiscStorage::convertFromStateString(
    const std::string& stateString,
    SpeakerManagerStorageState::ChannelState& state) noexcept {
    rapidjson::Document document;
    if (!jsonUtils::parseJSON(stateString, &document)) {
        ACSDK_ERROR(LX("convertFromStateString").d("reason", "parsingError"));
        return false;
    }

    int64_t tmpVolume;

    if (jsonUtils::retrieveValue(document, CHANNEL_VOLUME_KEY, &tmpVolume)) {
        state.channelVolume = static_cast<uint8_t>(tmpVolume);
    } else {
        return false;
    }
    if (jsonUtils::retrieveValue(document, CHANNEL_MUTE_STATUS_KEY, &state.channelMuteStatus)) {
        return true;
    }

    return false;
}

bool SpeakerManagerMiscStorage::convertFromStateString(
    const std::string& stateString,
    SpeakerManagerStorageState& state) noexcept {
    std::string tmp;

    return jsonUtils::retrieveValue(stateString, SPEAKER_CHANNEL_STATE, &tmp) &&
           convertFromStateString(tmp, state.speakerChannelState) &&
           jsonUtils::retrieveValue(stateString, ALERTS_CHANNEL_STATE, &tmp) &&
           convertFromStateString(tmp, state.alertsChannelState);
}

bool SpeakerManagerMiscStorage::loadState(SpeakerManagerStorageState& state) noexcept {
    std::string stateString;

    return m_miscStorage->get(COMPONENT_NAME, COMPONENT_STATE_TABLE, COMPONENT_STATE_KEY, &stateString) &&
           !stateString.empty() && convertFromStateString(stateString, state);
}

std::string SpeakerManagerMiscStorage::convertToStateString(
    const SpeakerManagerStorageState::ChannelState& state) noexcept {
    JsonGenerator generator;
    generator.addMember(CHANNEL_VOLUME_KEY, state.channelVolume);
    generator.addMember(CHANNEL_MUTE_STATUS_KEY, state.channelMuteStatus);
    return generator.toString();
}

std::string SpeakerManagerMiscStorage::convertToStateString(const SpeakerManagerStorageState& state) noexcept {
    ACSDK_DEBUG5(LX(__func__));
    JsonGenerator generator;
    generator.addRawJsonMember(SPEAKER_CHANNEL_STATE, convertToStateString(state.speakerChannelState));
    generator.addRawJsonMember(ALERTS_CHANNEL_STATE, convertToStateString(state.alertsChannelState));
    return generator.toString();
}

bool SpeakerManagerMiscStorage::saveState(const SpeakerManagerStorageState& state) noexcept {
    std::string stateString = convertToStateString(state);
    if (!m_miscStorage->put(COMPONENT_NAME, COMPONENT_STATE_TABLE, COMPONENT_STATE_KEY, stateString)) {
        ACSDK_ERROR(LX("saveStateFailed")
                        .d("reason", "Unable to update the table")
                        .d("table", COMPONENT_STATE_TABLE)
                        .d("key", COMPONENT_STATE_KEY)
                        .d("component", COMPONENT_NAME));
        return false;
    }
    return true;
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
