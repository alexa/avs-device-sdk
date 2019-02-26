/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Alerts/Alert.h"

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <limits>
#include <string>
#include <time.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

using namespace avsCommon::avs;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace avsCommon::utils::timing;
using namespace renderer;
using namespace rapidjson;

/// String for lookup of the token value in a parsed JSON document.
static const std::string KEY_TOKEN = "token";
/// String for lookup of the scheduled time value in a parsed JSON document.
static const std::string KEY_SCHEDULED_TIME = "scheduledTime";
/// String for lookup of the assets array in a parsed JSON document.
static const std::string KEY_ASSETS = "assets";
/// String for lookup of the asset id within an asset object in a parsed JSON document.
static const std::string KEY_ASSET_ID = "assetId";
/// String for lookup of the asset url within an asset object in a parsed JSON document.
static const std::string KEY_ASSET_URL = "url";
/// String for lookup of the asset play order array in a parsed JSON document.
static const std::string KEY_ASSET_PLAY_ORDER = "assetPlayOrder";
/// String for lookup of the loop count value in a parsed JSON document.
static const std::string KEY_LOOP_COUNT = "loopCount";
/// String for lookup of the loop pause in milliseconds value in a parsed JSON document.
static const std::string KEY_LOOP_PAUSE_IN_MILLISECONDS = "loopPauseInMilliSeconds";
/// String for lookup of the backgroundAssetId for an alert, if assets are provided.
static const std::string KEY_BACKGROUND_ASSET_ID = "backgroundAlertAsset";

/// We won't allow an alert to render more than 1 hour.
const std::chrono::seconds MAXIMUM_ALERT_RENDERING_TIME = std::chrono::hours(1);
/// Length of pause of alert sounds when played in background
const auto BACKGROUND_ALERT_SOUND_PAUSE_TIME = std::chrono::seconds(10);

/// String to identify log entries originating from this file.
static const std::string TAG("Alert");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

Alert::Alert(
    std::function<std::unique_ptr<std::istream>()> defaultAudioFactory,
    std::function<std::unique_ptr<std::istream>()> shortAudioFactory) :
        m_stopReason{StopReason::UNSET},
        m_focusState{avsCommon::avs::FocusState::NONE},
        m_hasTimerExpired{false},
        m_observer{nullptr},
        m_defaultAudioFactory{defaultAudioFactory},
        m_shortAudioFactory{shortAudioFactory} {
}

/**
 * Utility function to parse the optional Asset data from an AVS SetAlert Directive.  The only strictly required
 * fields for assets to be valid are the assets themselves (a pair of id & url), and the assetPlayOrder.  If the
 * other fields are missing or empty, they are ignored.  If the assets are malformed or missing, or otherwise
 * not complete for asset rendering, this function returns early allowing the caller to proceed as if there were no
 * assets in the Directive.  This allows the Alert to still serve its purpose in some capacity.
 *
 * @param payload The pre-parsed rapidjson::Value.
 * @param[out] assetConfiguration An output parameter where the assets will be stored if parsed successfully.
 * @return The status of the parse.
 */
static Alert::ParseFromJsonStatus parseAlertAssetConfigurationFromJson(
    const rapidjson::Value& payload,
    Alert::AssetConfiguration* assetConfiguration,
    Alert::DynamicData* dynamicData) {
    if (!assetConfiguration) {
        ACSDK_ERROR(LX("parseAlertAssetConfigurationFromJson : assetConfiguration is nullptr."));
        return Alert::ParseFromJsonStatus::OK;
    }
    if (!dynamicData) {
        ACSDK_ERROR(LX("parseAlertAssetConfigurationFromJson : dynamicData is nullptr."));
        return Alert::ParseFromJsonStatus::OK;
    }

    Alert::AssetConfiguration localAssetConfig;
    bool assetInfoFoundOk = true;
    int64_t loopCount = std::numeric_limits<int>::max();
    int64_t loopPauseInMilliseconds = 0;
    std::string backgroundAssetId;

    if (!jsonArrayExists(payload, KEY_ASSETS)) {
        ACSDK_DEBUG0(LX("parseAlertAssetConfigurationFromJson : assets are not present."));
        assetInfoFoundOk = false;
    }
    if (!jsonArrayExists(payload, KEY_ASSET_PLAY_ORDER)) {
        ACSDK_DEBUG0(LX("parseAlertAssetConfigurationFromJson : asset play order is not present."));
        assetInfoFoundOk = false;
    }
    if (!retrieveValue(payload, KEY_LOOP_COUNT, &loopCount)) {
        ACSDK_DEBUG0(LX("parseAlertAssetConfigurationFromJson : loop count is not present."));
        /*
         * It's ok if this is not set.  Defaults to std::numeric_limits<int>::max() and stop it using
         * onMaxTimerExpiration(), which per AVS means we render for the full hour.
         */
    }
    if (!retrieveValue(payload, KEY_LOOP_PAUSE_IN_MILLISECONDS, &loopPauseInMilliseconds)) {
        ACSDK_DEBUG0(LX("parseAlertAssetConfigurationFromJson : loop pause in milliseconds is not present."));
        // it's ok if this is not set.
    }
    if (!retrieveValue(payload, KEY_BACKGROUND_ASSET_ID, &backgroundAssetId)) {
        ACSDK_DEBUG0(LX("parseAlertAssetConfigurationFromJson : backgroundAssetId is not present."));
        // it's ok if this is not set.
    }

    // Something was missing, but these are optional fields - let's still allow the alert to be set.
    if (!assetInfoFoundOk) {
        return Alert::ParseFromJsonStatus::OK;
    }

    auto assetJsonArray = payload.FindMember(KEY_ASSETS.c_str())->value.GetArray();
    for (rapidjson::SizeType i = 0; i < assetJsonArray.Size(); i++) {
        Alert::Asset asset;
        if (!retrieveValue(assetJsonArray[i], KEY_ASSET_ID, &asset.id)) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : assetId is not present."));
            return Alert::ParseFromJsonStatus::OK;
        }
        if (!retrieveValue(assetJsonArray[i], KEY_ASSET_URL, &asset.url)) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : assetUrl is not present."));
            return Alert::ParseFromJsonStatus::OK;
        }

        // the id and url strings must have content.
        if (asset.id.empty() || asset.url.empty()) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : invalid asset data."));
            return Alert::ParseFromJsonStatus::OK;
        }

        // duplicates aren't ok
        if (assetConfiguration->assets.find(asset.id) != assetConfiguration->assets.end()) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : duplicate assetId detected."));
            return Alert::ParseFromJsonStatus::OK;
        }

        localAssetConfig.assets[asset.id] = asset;
    }

    auto playOrderItemsJsonArray = payload.FindMember(KEY_ASSET_PLAY_ORDER.c_str())->value.GetArray();
    for (rapidjson::SizeType i = 0; i < playOrderItemsJsonArray.Size(); i++) {
        if (!playOrderItemsJsonArray[i].IsString()) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : invalid play order item type detected."));
            return Alert::ParseFromJsonStatus::OK;
        }

        auto assetId = playOrderItemsJsonArray[i].GetString();

        if (localAssetConfig.assets.find(assetId) == localAssetConfig.assets.end()) {
            ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson : invalid play order item - asset does not exist."));
            return Alert::ParseFromJsonStatus::OK;
        }

        localAssetConfig.assetPlayOrderItems.push_back(assetId);
    }

    if (loopCount > std::numeric_limits<int>::max()) {
        ACSDK_WARN(LX("parseAlertAssetConfigurationFromJson")
                       .d("loopCountValue", loopCount)
                       .m("loopCount cannot be converted to integer."));
        return Alert::ParseFromJsonStatus::OK;
    }

    localAssetConfig.loopPause = std::chrono::milliseconds{loopPauseInMilliseconds};
    localAssetConfig.backgroundAssetId = backgroundAssetId;

    // ok, great - let's assign to the out parameter.
    *assetConfiguration = localAssetConfig;
    dynamicData->loopCount = static_cast<int>(loopCount);

    return Alert::ParseFromJsonStatus::OK;
}

Alert::ParseFromJsonStatus Alert::parseFromJson(const rapidjson::Value& payload, std::string* errorMessage) {
    if (!retrieveValue(payload, KEY_TOKEN, &m_staticData.token)) {
        ACSDK_ERROR(LX("parseFromJsonFailed").m("could not parse token."));
        *errorMessage = "missing property: " + KEY_TOKEN;
        return ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY;
    }

    std::string parsedScheduledTime_ISO_8601;
    if (!retrieveValue(payload, KEY_SCHEDULED_TIME, &parsedScheduledTime_ISO_8601)) {
        ACSDK_ERROR(LX("parseFromJsonFailed").m("could not parse scheduled time."));
        *errorMessage = "missing property: " + KEY_SCHEDULED_TIME;
        return ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY;
    }

    if (!m_dynamicData.timePoint.setTime_ISO_8601(parsedScheduledTime_ISO_8601)) {
        ACSDK_ERROR(LX("parseFromJsonFailed")
                        .m("could not convert time to unix.")
                        .d("parsed time string", parsedScheduledTime_ISO_8601));
        return ParseFromJsonStatus::INVALID_VALUE;
    }

    return parseAlertAssetConfigurationFromJson(payload, &m_staticData.assetConfiguration, &m_dynamicData);
}

void Alert::setRenderer(std::shared_ptr<renderer::RendererInterface> renderer) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_renderer) {
        ACSDK_ERROR(LX("setRendererFailed").m("Renderer is already set."));
        return;
    }
    m_renderer = renderer;
}

void Alert::setObserver(AlertObserverInterface* observer) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_observer = observer;
}

void Alert::setFocusState(FocusState focusState) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto rendererCopy = m_renderer;
    auto alertState = m_dynamicData.state;

    if (focusState == m_focusState) {
        return;
    }

    m_focusState = focusState;

    lock.unlock();

    if (State::ACTIVE == alertState) {
        rendererCopy->stop();
        startRenderer();
    }
}

bool Alert::setStateActive() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (State::ACTIVATING != m_dynamicData.state) {
        ACSDK_ERROR(LX("setStateActiveFailed").d("current state", stateToString(m_dynamicData.state)));
        return false;
    }

    m_dynamicData.state = State::ACTIVE;
    return true;
}

void Alert::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dynamicData.state = Alert::State::SET;
}

void Alert::activate() {
    ACSDK_DEBUG9(LX("activate"));
    std::unique_lock<std::mutex> lock(m_mutex);

    if (Alert::State::ACTIVATING == m_dynamicData.state || Alert::State::ACTIVE == m_dynamicData.state) {
        ACSDK_ERROR(LX("activateFailed").m("Alert is already active."));
        return;
    }

    m_dynamicData.state = Alert::State::ACTIVATING;

    if (!m_maxLengthTimer.isActive()) {
        if (!m_maxLengthTimer.start(MAXIMUM_ALERT_RENDERING_TIME, std::bind(&Alert::onMaxTimerExpiration, this))
                 .valid()) {
            ACSDK_ERROR(LX("executeStartFailed").d("reason", "startTimerFailed"));
        }
    }

    lock.unlock();

    startRenderer();
}

void Alert::deactivate(StopReason reason) {
    ACSDK_DEBUG9(LX("deactivate").d("reason", reason));
    std::unique_lock<std::mutex> lock(m_mutex);
    auto rendererCopy = m_renderer;

    m_dynamicData.state = Alert::State::STOPPING;
    m_stopReason = reason;
    m_maxLengthTimer.stop();

    lock.unlock();

    rendererCopy->stop();
}

void Alert::getAlertData(StaticData* staticData, DynamicData* dynamicData) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!dynamicData && !staticData) {
        return;
    }

    if (staticData) {
        *staticData = m_staticData;
    }

    if (dynamicData) {
        *dynamicData = m_dynamicData;
    }
}

/**
 * Checks if the assets map contains a certain key and that the object mapped by the key is valid.
 *
 * @param key The key to search in the assets map
 * @param assets The assets map
 * @return @c true if the key was found in the assets, if the asset's id is the same as the key and asset url is not
 * empty
 */
static bool hasAsset(const std::string& key, const std::unordered_map<std::string, Alert::Asset>& assets) {
    auto search = assets.find(key);
    return (search != assets.end()) && (search->second.id == key) && (!search->second.url.empty());
}

/**
 * Validates the static data
 *
 * @param staticData Static data struct to validate
 * @return @c true if the data is valid
 */
static bool validateStaticData(const Alert::StaticData& staticData) {
    if (!staticData.assetConfiguration.backgroundAssetId.empty() &&
        !hasAsset(staticData.assetConfiguration.backgroundAssetId, staticData.assetConfiguration.assets)) {
        ACSDK_ERROR(LX("validateStaticDataFailed")
                        .d("reason", "invalidStaticData")
                        .d("assetId", staticData.assetConfiguration.backgroundAssetId)
                        .m("backgroundAssetId is not represented in the list of assets"));
        return false;
    }
    for (std::string const& a : staticData.assetConfiguration.assetPlayOrderItems) {
        if (!hasAsset(a, staticData.assetConfiguration.assets)) {
            ACSDK_ERROR(LX("validateStaticDataFailed")
                            .d("reason", "invalidStaticData")
                            .d("assetId", a)
                            .m("Asset ID on assetPlayOrderItems is not represented in the list of assets"));
            return false;
        }
    }
    return true;
}

bool Alert::setAlertData(StaticData* staticData, DynamicData* dynamicData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!dynamicData && !staticData) {
        return false;
    }

    if (staticData) {
        if (!validateStaticData(*staticData)) {
            ACSDK_ERROR(LX("setAlertDataFailed").d("reason", "validateStaticDataFailed"));
            return false;
        }
        m_staticData = *staticData;
    }

    if (dynamicData) {
        if (dynamicData->loopCount < 0) {
            ACSDK_ERROR(
                LX("setAlertDataFailed").d("loopCountValue", dynamicData->loopCount).m("loopCount less than zero."));
            return false;
        }
        m_dynamicData = *dynamicData;
    }
    return true;
}

void Alert::onRendererStateChange(RendererObserverInterface::State state, const std::string& reason) {
    ACSDK_DEBUG1(LX("onRendererStateChange")
                     .d("state", state)
                     .d("reason", reason)
                     .d("m_hasTimerExpired", m_hasTimerExpired)
                     .d("m_dynamicData.state", m_dynamicData.state));
    bool shouldNotifyObserver = false;
    bool shouldRetryRendering = false;
    AlertObserverInterface::State notifyState = AlertObserverInterface::State::ERROR;
    std::string notifyReason;

    std::unique_lock<std::mutex> lock(m_mutex);
    auto observerCopy = m_observer;

    switch (state) {
        case RendererObserverInterface::State::UNSET:
            // no-op
            break;

        case RendererObserverInterface::State::STARTED:
            if (State::ACTIVATING == m_dynamicData.state) {
                shouldNotifyObserver = true;
                notifyState = AlertObserverInterface::State::STARTED;

                // NOTE : we don't update state to ACTIVE here.  We leave it as ACTIVATED, allowing our owning
                // object to set the state to active when it chooses to.
            }
            break;

        case RendererObserverInterface::State::STOPPED:
            if (m_hasTimerExpired) {
                m_dynamicData.state = State::COMPLETED;
                shouldNotifyObserver = true;
                notifyState = AlertObserverInterface::State::COMPLETED;
            } else {
                if (Alert::State::STOPPING == m_dynamicData.state) {
                    m_dynamicData.state = State::STOPPED;
                    shouldNotifyObserver = true;
                    notifyState = AlertObserverInterface::State::STOPPED;
                    notifyReason = stopReasonToString(m_stopReason);
                } else if (Alert::State::SNOOZING == m_dynamicData.state) {
                    m_dynamicData.state = State::SNOOZED;
                    shouldNotifyObserver = true;
                    notifyState = AlertObserverInterface::State::SNOOZED;
                }
            }
            break;

        case RendererObserverInterface::State::COMPLETED:
            m_dynamicData.state = State::COMPLETED;
            shouldNotifyObserver = true;
            notifyState = AlertObserverInterface::State::COMPLETED;
            break;

        case RendererObserverInterface::State::ERROR:
            // If the renderer failed while handling a url, let's presume there are network issues and render
            // the on-device background audio sound instead.
            if (!m_staticData.assetConfiguration.assetPlayOrderItems.empty() && !m_dynamicData.hasRenderingFailed) {
                ACSDK_ERROR(LX("onRendererStateChangeFailed")
                                .d("reason", reason)
                                .m("Renderer failed to handle a url. Retrying with local background audio sound."));
                m_dynamicData.hasRenderingFailed = true;
                shouldRetryRendering = true;
            } else {
                shouldNotifyObserver = true;
                notifyState = AlertObserverInterface::State::ERROR;
                notifyReason = reason;
            }
            break;
    }

    lock.unlock();

    if (shouldNotifyObserver && observerCopy) {
        observerCopy->onAlertStateChange(m_staticData.token, getTypeName(), notifyState, notifyReason);
    }

    if (shouldRetryRendering) {
        startRenderer();
    }
}

std::string Alert::getToken() const {
    return m_staticData.token;
}

int64_t Alert::getScheduledTime_Unix() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dynamicData.timePoint.getTime_Unix();
}

std::string Alert::getScheduledTime_ISO_8601() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getScheduledTime_ISO_8601Locked();
}

std::string Alert::getScheduledTime_ISO_8601Locked() const {
    return m_dynamicData.timePoint.getTime_ISO_8601();
}

Alert::State Alert::getState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dynamicData.state;
}

int Alert::getId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_staticData.dbId;
}

bool Alert::updateScheduledTime(const std::string& newScheduledTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto state = m_dynamicData.state;
    if (State::ACTIVE == state || State::ACTIVATING == state || State::STOPPING == state) {
        ACSDK_ERROR(LX("updateScheduledTimeFailed").d("reason", "unexpectedState").d("state", m_dynamicData.state));
        return false;
    }

    if (!m_dynamicData.timePoint.setTime_ISO_8601(newScheduledTime)) {
        ACSDK_ERROR(LX("updateScheduledTimeFailed").d("reason", "setTimeFailed").d("newTime", newScheduledTime));
        return false;
    }

    m_dynamicData.state = State::SET;

    return true;
}

bool Alert::snooze(const std::string& updatedScheduledTime) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto rendererCopy = m_renderer;

    if (!m_dynamicData.timePoint.setTime_ISO_8601(updatedScheduledTime)) {
        ACSDK_ERROR(LX("snoozeFailed").d("reason", "setTimeFailed").d("updatedScheduledTime", updatedScheduledTime));
        return false;
    }

    m_dynamicData.state = State::SNOOZING;
    lock.unlock();

    rendererCopy->stop();
    return true;
}

Alert::StopReason Alert::getStopReason() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stopReason;
}

int Alert::getLoopCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dynamicData.loopCount;
}

std::chrono::milliseconds Alert::getLoopPause() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_staticData.assetConfiguration.loopPause;
}

std::string Alert::getBackgroundAssetId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_staticData.assetConfiguration.backgroundAssetId;
}

Alert::AssetConfiguration Alert::getAssetConfiguration() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_staticData.assetConfiguration;
}

void Alert::startRenderer() {
    ACSDK_DEBUG9(LX("startRenderer"));
    std::vector<std::string> urls;

    std::unique_lock<std::mutex> lock(m_mutex);
    auto rendererCopy = m_renderer;
    auto loopCount = m_dynamicData.loopCount;
    auto loopPause = m_staticData.assetConfiguration.loopPause;

    // If there are no assets to play (due to the alert not providing any assets), or there was a previous error
    // (indicated by m_staticData.assetConfiguration.hasRenderingFailed), we call rendererCopy->start(..) with an
    // empty vector of urls.  This causes the default audio to be rendered.
    auto audioFactory = getDefaultAudioFactory();
    if (avsCommon::avs::FocusState::BACKGROUND == m_focusState) {
        audioFactory = getShortAudioFactory();
        if (!m_staticData.assetConfiguration.backgroundAssetId.empty() && !m_dynamicData.hasRenderingFailed) {
            urls.push_back(
                m_staticData.assetConfiguration.assets[m_staticData.assetConfiguration.backgroundAssetId].url);
        }
        loopPause = BACKGROUND_ALERT_SOUND_PAUSE_TIME;
    } else if (!m_staticData.assetConfiguration.assets.empty() && !m_dynamicData.hasRenderingFailed) {
        // Only play the named timer urls when it's in foreground.
        for (auto item : m_staticData.assetConfiguration.assetPlayOrderItems) {
            urls.push_back(m_staticData.assetConfiguration.assets[item].url);
        }
    }

    lock.unlock();

    rendererCopy->start(shared_from_this(), audioFactory, urls, loopCount, loopPause);
}

void Alert::onMaxTimerExpiration() {
    ACSDK_DEBUG1(LX("onMaxTimerExpiration"));
    std::unique_lock<std::mutex> lock(m_mutex);
    auto rendererCopy = m_renderer;
    m_dynamicData.state = Alert::State::STOPPING;
    m_hasTimerExpired = true;

    lock.unlock();

    rendererCopy->stop();
}

bool Alert::isPastDue(int64_t currentUnixTime, std::chrono::seconds timeLimit) {
    std::lock_guard<std::mutex> lock(m_mutex);

    int64_t cutoffTime = currentUnixTime - timeLimit.count();

    return (m_dynamicData.timePoint.getTime_Unix() < cutoffTime);
}

std::function<std::unique_ptr<std::istream>()> Alert::getDefaultAudioFactory() const {
    return m_defaultAudioFactory;
}

std::function<std::unique_ptr<std::istream>()> Alert::getShortAudioFactory() const {
    return m_shortAudioFactory;
}

Alert::ContextInfo Alert::getContextInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    return ContextInfo(m_staticData.token, getTypeName(), getScheduledTime_ISO_8601Locked());
}

std::string Alert::stateToString(Alert::State state) {
    switch (state) {
        case Alert::State::UNSET:
            return "UNSET";
        case Alert::State::SET:
            return "SET";
        case Alert::State::READY:
            return "READY";
        case Alert::State::ACTIVATING:
            return "ACTIVATING";
        case Alert::State::ACTIVE:
            return "ACTIVE";
        case Alert::State::SNOOZING:
            return "SNOOZING";
        case Alert::State::SNOOZED:
            return "SNOOZED";
        case Alert::State::STOPPING:
            return "STOPPING";
        case Alert::State::STOPPED:
            return "STOPPED";
        case Alert::State::COMPLETED:
            return "COMPLETED";
    }

    ACSDK_ERROR(LX("stateToStringFailed").d("unhandledCase", state));

    return "UNKNOWN_STATE";
}

std::string Alert::stopReasonToString(Alert::StopReason stopReason) {
    switch (stopReason) {
        case Alert::StopReason::UNSET:
            return "UNSET";
        case Alert::StopReason::AVS_STOP:
            return "AVS_STOP";
        case Alert::StopReason::LOCAL_STOP:
            return "LOCAL_STOP";
        case Alert::StopReason::SHUTDOWN:
            return "SHUTDOWN";
        case Alert::StopReason::LOG_OUT:
            return "LOG_OUT";
    }

    ACSDK_ERROR(LX("stopReasonToStringFailed").d("unhandledCase", stopReason));

    return "UNKNOWN_STOP_REASON";
}

std::string Alert::parseFromJsonStatusToString(Alert::ParseFromJsonStatus parseFromJsonStatus) {
    switch (parseFromJsonStatus) {
        case Alert::ParseFromJsonStatus::OK:
            return "OK";
        case Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY:
            return "MISSING_REQUIRED_PROPERTY";
        case Alert::ParseFromJsonStatus::INVALID_VALUE:
            return "INVALID_VALUE";
    }

    ACSDK_ERROR(LX("parseFromJsonStatusToStringFailed").d("unhandledCase", parseFromJsonStatus));

    return "UNKNOWN_PARSE_FROM_JSON_STATUS";
}

void Alert::printDiagnostic() {
    std::string assetInfoString;
    for (auto asset : m_staticData.assetConfiguration.assets) {
        assetInfoString += "\nid:" + asset.second.id + ", url:" + asset.second.url;
    }

    std::string assetPlayOrderItemsInfoString;
    for (auto assetOrderItem : m_staticData.assetConfiguration.assetPlayOrderItems) {
        assetPlayOrderItemsInfoString += "id:" + assetOrderItem + ", ";
    }

    std::stringstream ss;

    ss << std::endl
       << " ** Alert | id:" << std::to_string(m_staticData.dbId) << std::endl
       << "          | type:" << getTypeName() << std::endl
       << "          | token:" << m_staticData.token << std::endl
       << "          | scheduled time (8601):" << getScheduledTime_ISO_8601() << std::endl
       << "          | scheduled time (Unix):" << getScheduledTime_Unix() << std::endl
       << "          | state:" << stateToString(m_dynamicData.state) << std::endl
       << "          | number assets:" << m_staticData.assetConfiguration.assets.size() << std::endl
       << "          | number assets play order items:" << m_staticData.assetConfiguration.assetPlayOrderItems.size()
       << std::endl
       << "          | asset info:" << assetInfoString << std::endl
       << "          | asset order info:" << assetPlayOrderItemsInfoString << std::endl
       << "          | background asset id:" << m_staticData.assetConfiguration.backgroundAssetId << std::endl
       << "          | loop count:" << m_dynamicData.loopCount << std::endl
       << "          | loop pause in milliseconds:" << m_staticData.assetConfiguration.loopPause.count() << std::endl;

    ACSDK_INFO(LX(ss.str()));
}

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
