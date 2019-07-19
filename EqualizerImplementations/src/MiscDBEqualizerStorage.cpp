/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <memory>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "EqualizerImplementations/MiscDBEqualizerStorage.h"
#include "EqualizerImplementations/EqualizerUtils.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::utils;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::string;

/// String to identify log entries originating from this file.
static const std::string TAG{"MiscDBEqualizerStorage"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Component name needed for Misc DB
static const std::string COMPONENT_NAME = "equalizerController";
/// Misc DB table for equalizer state
static const std::string EQUALIZER_STATE_TABLE = "equalizerState";
/// Key for equalizer state in Misc DB table
static const std::string EQUALIZER_STATE_KEY = "state";

void MiscDBEqualizerStorage::saveState(const EqualizerState& state) {
    std::string stateStr = EqualizerUtils::serializeEqualizerState(state);
    if (!m_miscStorage->put(COMPONENT_NAME, EQUALIZER_STATE_TABLE, EQUALIZER_STATE_KEY, stateStr)) {
        ACSDK_ERROR(LX("saveStateFailed")
                        .d("reason", "Unable to update the table")
                        .d("table", EQUALIZER_STATE_TABLE)
                        .d("component", COMPONENT_NAME));
        ACSDK_DEBUG3(LX(__func__).m("Clearing table"));
        if (!m_miscStorage->clearTable(COMPONENT_NAME, EQUALIZER_STATE_TABLE)) {
            ACSDK_ERROR(LX("saveStateFailed")
                            .d("reason", "Unable to clear the table")
                            .d("table", EQUALIZER_STATE_TABLE)
                            .d("component", COMPONENT_NAME)
                            .m("Please clear the table for proper future functioning."));
        }
    }
}

SuccessResult<EqualizerState> MiscDBEqualizerStorage::loadState() {
    std::string stateString;
    m_miscStorage->get(COMPONENT_NAME, EQUALIZER_STATE_TABLE, EQUALIZER_STATE_KEY, &stateString);

    if (stateString.empty()) {
        return SuccessResult<EqualizerState>::failure();
    }

    return EqualizerUtils::deserializeEqualizerState(stateString);
}

void MiscDBEqualizerStorage::clear() {
    ACSDK_DEBUG5(LX(__func__));
    bool capabilitiesTableExists = false;
    if (m_miscStorage->tableExists(COMPONENT_NAME, EQUALIZER_STATE_TABLE, &capabilitiesTableExists)) {
        if (capabilitiesTableExists) {
            if (!m_miscStorage->clearTable(COMPONENT_NAME, EQUALIZER_STATE_TABLE)) {
                ACSDK_ERROR(LX("clearFailed")
                                .d("reason", "Unable to clear the table")
                                .d("table", EQUALIZER_STATE_TABLE)
                                .d("component", COMPONENT_NAME)
                                .m("Please clear the table for proper future functioning."));
            } else if (!m_miscStorage->deleteTable(COMPONENT_NAME, EQUALIZER_STATE_TABLE)) {
                ACSDK_ERROR(LX("clearFailed")
                                .d("reason", "Unable to delete the table")
                                .d("table", EQUALIZER_STATE_TABLE)
                                .d("component", COMPONENT_NAME)
                                .m("Please delete the table for proper future functioning."));
            }
        }
    } else {
        ACSDK_ERROR(LX("clearFailed")
                        .d("reason", "Unable to check if table exists")
                        .d("table", EQUALIZER_STATE_TABLE)
                        .d("component", COMPONENT_NAME)
                        .m("Please delete the table for proper future functioning."));
    }
}

std::shared_ptr<MiscDBEqualizerStorage> MiscDBEqualizerStorage::create(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage) {
    if (nullptr == storage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "storageNull"));
        return nullptr;
    }
    auto equalizerStorage = std::shared_ptr<MiscDBEqualizerStorage>(new MiscDBEqualizerStorage(storage));
    if (!equalizerStorage->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Failed to initialize."));
        return nullptr;
    }

    return equalizerStorage;
}

MiscDBEqualizerStorage::MiscDBEqualizerStorage(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage) :
        m_miscStorage{storage} {
}

bool MiscDBEqualizerStorage::initialize() {
    if (!m_miscStorage->isOpened() && !m_miscStorage->open()) {
        ACSDK_DEBUG3(LX(__func__).m("Couldn't open misc database. Creating."));
        if (!m_miscStorage->createDatabase()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not create misc database."));
            return false;
        }
    }

    bool tableExists = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, EQUALIZER_STATE_TABLE, &tableExists)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not get Capabilities table information misc database."));
        return false;
    }

    if (!tableExists) {
        ACSDK_DEBUG3(LX(__func__).m("Table doesn't exist in misc database. Creating new."));
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                EQUALIZER_STATE_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "Cannot create table")
                            .d("table", EQUALIZER_STATE_TABLE)
                            .d("component", COMPONENT_NAME));
            return false;
        }
    }

    return true;
}

}  // namespace equalizer
}  // namespace alexaClientSDK
