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

#include "AVSGatewayManager/Storage/AVSGatewayManagerStorage.h"

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace storage {

using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
static const std::string TAG("AVSGatewayManagerStorage");

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Component name for Misc DB.
static const std::string COMPONENT_NAME = "avsGatewayManager";

/// Misc DB table for Verification State.
static const std::string VERIFICATION_STATE_TABLE = "verificationState";

/// Key for state in Misc DB table.
static const std::string VERIFICATION_STATE_KEY = "state";

/// Json key for gateway URL.
static const std::string GATEWAY_URL_KEY = "gatewayURL";

/// Json key for isGatewayVerified.
static const std::string IS_VERIFIED_KEY = "isVerified";

std::unique_ptr<AVSGatewayManagerStorage> AVSGatewayManagerStorage::create(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) {
    if (!miscStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMiscStorage"));
    } else {
        return std::unique_ptr<AVSGatewayManagerStorage>(new AVSGatewayManagerStorage(miscStorage));
    }
    return nullptr;
}

AVSGatewayManagerStorage::AVSGatewayManagerStorage(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) :
        m_miscStorage{miscStorage} {
}

bool AVSGatewayManagerStorage::init() {
    if (!m_miscStorage->isOpened() && !m_miscStorage->open()) {
        ACSDK_DEBUG3(LX(__func__).m("Couldn't open misc database. Creating."));
        if (!m_miscStorage->createDatabase()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not create misc database."));
            return false;
        }
    }

    bool tableExists = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "Could not check state table information in misc database."));
        return false;
    }

    if (!tableExists) {
        ACSDK_DEBUG3(LX(__func__).m("Table doesn't exist in misc database. Creating new."));
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                VERIFICATION_STATE_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "Cannot create table")
                            .d("table", VERIFICATION_STATE_TABLE)
                            .d("component", COMPONENT_NAME));
            return false;
        }
    }
    return true;
}

std::string convertToStateString(const GatewayVerifyState& state) {
    ACSDK_DEBUG5(LX(__func__));
    JsonGenerator generator;
    generator.addMember(GATEWAY_URL_KEY, state.avsGatewayURL);
    generator.addMember(IS_VERIFIED_KEY, state.isVerified);
    return generator.toString();
}

bool convertFromStateString(const std::string& stateString, GatewayVerifyState* state) {
    if (!state) {
        return false;
    }
    if (!jsonUtils::retrieveValue(stateString, GATEWAY_URL_KEY, &state->avsGatewayURL)) {
        return false;
    }
    if (!jsonUtils::retrieveValue(stateString, IS_VERIFIED_KEY, &state->isVerified)) {
        return false;
    }
    return true;
}

bool AVSGatewayManagerStorage::loadState(GatewayVerifyState* state) {
    if (!state) {
        return false;
    }
    std::string stateString;
    if (!m_miscStorage->get(COMPONENT_NAME, VERIFICATION_STATE_TABLE, VERIFICATION_STATE_KEY, &stateString)) {
        return false;
    }

    if (!stateString.empty()) {
        return convertFromStateString(stateString, state);
    }

    return true;
}

bool AVSGatewayManagerStorage::saveState(const GatewayVerifyState& state) {
    std::string stateString = convertToStateString(state);
    if (!m_miscStorage->put(COMPONENT_NAME, VERIFICATION_STATE_TABLE, VERIFICATION_STATE_KEY, stateString)) {
        ACSDK_ERROR(LX("saveStateFailed")
                        .d("reason", "Unable to update the table")
                        .d("table", VERIFICATION_STATE_TABLE)
                        .d("component", COMPONENT_NAME));
        return false;
    }
    return true;
}

void AVSGatewayManagerStorage::clear() {
    ACSDK_DEBUG5(LX(__func__));
    bool verificationStateTableExists = false;
    if (m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &verificationStateTableExists)) {
        if (verificationStateTableExists) {
            if (!m_miscStorage->clearTable(COMPONENT_NAME, VERIFICATION_STATE_TABLE)) {
                ACSDK_ERROR(LX("clearFailed")
                                .d("reason", "Unable to clear the table")
                                .d("table", VERIFICATION_STATE_TABLE)
                                .d("component", COMPONENT_NAME)
                                .m("Please clear the table for proper future functioning."));
            } else if (!m_miscStorage->deleteTable(COMPONENT_NAME, VERIFICATION_STATE_TABLE)) {
                ACSDK_ERROR(LX("clearFailed")
                                .d("reason", "Unable to delete the table")
                                .d("table", VERIFICATION_STATE_TABLE)
                                .d("component", COMPONENT_NAME)
                                .m("Please delete the table for proper future functioning."));
            }
        }
    } else {
        ACSDK_ERROR(LX("clearFailed")
                        .d("reason", "Unable to check if table exists")
                        .d("table", VERIFICATION_STATE_TABLE)
                        .d("component", COMPONENT_NAME)
                        .m("Please delete the table for proper future functioning."));
    }
}
}  // namespace storage
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
