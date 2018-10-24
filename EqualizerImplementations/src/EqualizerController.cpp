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

#include <algorithm>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "EqualizerImplementations/EqualizerController.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::error;
using namespace avsCommon::sdkInterfaces::audio;

/// String to identify log entries originating from this file.
static const std::string TAG{"EqualizerController"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<EqualizerController> EqualizerController::create(
    std::shared_ptr<EqualizerModeControllerInterface> modeController,
    std::shared_ptr<EqualizerConfigurationInterface> configuration,
    std::shared_ptr<EqualizerStorageInterface> storage) {
    if (nullptr == configuration) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullConfiguration"));
        return nullptr;
    }

    if (nullptr == storage) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullStorage"));
        return nullptr;
    }

    if (nullptr == modeController && !configuration->getSupportedModes().empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "modeController is null while there are modes supported."));
        return nullptr;
    }

    return std::shared_ptr<EqualizerController>(new EqualizerController(modeController, configuration, storage));
}

EqualizerController::EqualizerController(
    std::shared_ptr<EqualizerModeControllerInterface> modeController,
    std::shared_ptr<EqualizerConfigurationInterface> configuration,
    std::shared_ptr<EqualizerStorageInterface> storage) :
        m_modeController{modeController},
        m_configuration{configuration},
        m_storage{storage} {
    m_currentState = loadState();
}

EqualizerState EqualizerController::loadState() {
    SuccessResult<EqualizerState> loadedStateResult = m_storage->loadState();
    EqualizerState defaultState = m_configuration->getDefaultState();
    EqualizerState state = {};

    if (!loadedStateResult.isSucceeded()) {
        return defaultState;
    }

    auto loadedState = loadedStateResult.value();

    for (auto& it : defaultState.bandLevels) {
        EqualizerBand band = it.first;
        int bandLevel = it.second;
        auto storedBandIt = loadedState.bandLevels.find(band);
        if (loadedState.bandLevels.end() != storedBandIt) {
            bandLevel = storedBandIt->second;
        }
        state.bandLevels[band] = bandLevel;
    }

    return state;
}

SuccessResult<int> EqualizerController::getBandLevel(EqualizerBand band) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    auto iter = m_currentState.bandLevels.find(band);
    if (m_currentState.bandLevels.end() != iter) {
        return SuccessResult<int>::success(iter->second);
    }

    ACSDK_ERROR(LX("getBandLevelFailed").d("reason", "Invalid band requested"));
    return SuccessResult<int>::failure();
}

SuccessResult<avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap> EqualizerController::getBandLevels(
    std::set<EqualizerBand> bands) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    EqualizerBandLevelMap map;
    for (auto band : bands) {
        auto iter = m_currentState.bandLevels.find(band);
        if (m_currentState.bandLevels.end() != iter) {
            map[iter->first] = iter->second;
        }
    }

    if (map.size() != bands.size()) {
        ACSDK_ERROR(LX("getBandLevelsFailed").d("reason", "Invalid bands requested"));
        return SuccessResult<avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap>::failure();
    }

    return SuccessResult<avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap>::success(map);
}

int EqualizerController::truncateBandLevel(int level) {
    int maxLevel = m_configuration->getMaxBandLevel();
    int minLevel = m_configuration->getMinBandLevel();

    if (level > maxLevel) {
        ACSDK_DEBUG5(LX(__func__).m("Requested level is higher than maximum. Truncating."));
        return maxLevel;
    } else if (level < minLevel) {
        ACSDK_DEBUG5(LX(__func__).m("Requested level is lower than minimum. Truncating."));
        return minLevel;
    }

    return level;
}

void EqualizerController::setBandLevel(EqualizerBand band, int level) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    level = truncateBandLevel(level);

    auto iter = m_currentState.bandLevels.find(band);
    if (m_currentState.bandLevels.end() != iter) {
        if (iter->second == level) {
            return;
        }
        iter->second = level;
    } else {
        ACSDK_ERROR(LX("setBandLevelFailed").d("reason", "Invalid band requested"));
        return;
    }

    updateStateLocked();
}

void EqualizerController::setBandLevels(const EqualizerBandLevelMap& bandLevelMap) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    setBandLevelsLocked(bandLevelMap);
}

void EqualizerController::applyChangesToCurrentStateLocked(
    const EqualizerBandLevelMap& changesDataMap,
    std::function<int(int, int)> operation) {
    bool hasChanges = false;
    bool hasInvalidBands = false;

    for (auto changesIter : changesDataMap) {
        EqualizerBand band = changesIter.first;
        int changeValue = changesIter.second;
        auto stateIter = m_currentState.bandLevels.find(band);
        if (m_currentState.bandLevels.end() != stateIter) {
            int originalValue = stateIter->second;
            int newValue = operation(originalValue, changeValue);
            if (newValue != originalValue) {
                stateIter->second = newValue;
                hasChanges = true;
            }
        } else {
            hasInvalidBands = true;
        }
    }

    if (hasChanges) {
        updateStateLocked();
    }

    if (hasInvalidBands) {
        ACSDK_WARN(LX(__func__).m("Invalid bands requested"));
    }
}

void EqualizerController::setBandLevelsLocked(const EqualizerBandLevelMap& bandLevelMap) {
    applyChangesToCurrentStateLocked(bandLevelMap, [](int originalValue, int changeValue) { return changeValue; });
}

void EqualizerController::adjustBandLevels(const EqualizerBandLevelMap& bandAdjustmentMap) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    applyChangesToCurrentStateLocked(bandAdjustmentMap, [this](int originalValue, int changeValue) {
        return truncateBandLevel(originalValue + changeValue);
    });
}

void EqualizerController::resetBands(const std::set<EqualizerBand>& bands) {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    bool hasChanges = false;
    bool hasInvalidBands = false;

    EqualizerState defaultState = m_configuration->getDefaultState();

    for (auto band : bands) {
        auto iter = m_currentState.bandLevels.find(band);
        if (m_currentState.bandLevels.end() != iter) {
            // Assume that default state have the same configuration as the current
            int defaultLevel = defaultState.bandLevels[band];
            if (defaultLevel != iter->second) {
                iter->second = defaultLevel;
                hasChanges = true;
            }
        } else {
            hasInvalidBands = true;
        }
    }

    if (hasChanges) {
        updateStateLocked();
    }

    if (hasInvalidBands) {
        ACSDK_WARN(LX(__func__).m("Invalid bands requested"));
    }
}

EqualizerMode EqualizerController::getCurrentMode() {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    return m_currentState.mode;
}

void EqualizerController::setCurrentMode(EqualizerMode mode) {
    std::lock_guard<std::mutex> guard(m_modeMutex);

    if (!m_configuration->isModeSupported(mode)) {
        ACSDK_ERROR(LX("setCurrentModeFailed").d("reason", "Unsupported mode"));
        return;
    }

    if (nullptr == m_modeController) {
        ACSDK_ERROR(LX("setCurrentModeFailed")
                        .d("reason", "Configuration reports modes to be supported while no mode controller is set."));
        return;
    }

    if (m_currentState.mode == mode) {
        return;
    }

    if (!m_modeController->setEqualizerMode(mode)) {
        ACSDK_ERROR(LX("setCurrentModeFailed")
                        .d("reason", "setEqualizerMode() rejected the mode")
                        .d("mode", equalizerModeToString(mode)));
        return;
    }

    {
        std::lock_guard<std::mutex> stateGuard(m_stateMutex);
        m_currentState.mode = mode;
        updateStateLocked();
    }
}

void EqualizerController::registerEqualizer(std::shared_ptr<EqualizerInterface> equalizer) {
    if (nullptr == equalizer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEuqalizer"));
        return;
    }

    // Check if equalizer fully fits into configured band ranges and warn if it does not.
    int minLevel = m_configuration->getMinBandLevel();
    int maxLevel = m_configuration->getMaxBandLevel();
    if (equalizer->getMaximumBandLevel() < maxLevel) {
        ACSDK_WARN(LX("registerEqualizerMaxBandLevelInvalid")
                       .d("configuredMax", maxLevel)
                       .d("equalizerMax", equalizer->getMaximumBandLevel()));
    }
    if (equalizer->getMinimumBandLevel() > minLevel) {
        ACSDK_WARN(LX("registerEqualizerMinBandLevelInvalid")
                       .d("configuredMin", minLevel)
                       .d("equalizerMin", equalizer->getMinimumBandLevel()));
    }

    std::lock_guard<std::mutex> guard(m_stateMutex);

    m_equalizers.push_back(equalizer);
    equalizer->setEqualizerBandLevels(m_currentState.bandLevels);
}

void EqualizerController::unregisterEqualizer(std::shared_ptr<EqualizerInterface> equalizer) {
    if (nullptr == equalizer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEuqalizer"));
        return;
    }

    std::lock_guard<std::mutex> guard(m_stateMutex);

    auto iter = std::find(m_equalizers.begin(), m_equalizers.end(), equalizer);
    if (m_equalizers.end() != iter) {
        m_equalizers.erase(iter);
    }
}

void EqualizerController::addListener(std::shared_ptr<EqualizerControllerListenerInterface> listener) {
    if (nullptr == listener) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullListener"));
        return;
    }

    std::lock_guard<std::mutex> guard(m_stateMutex);

    m_listeners.push_back(listener);
}

void EqualizerController::removeListener(std::shared_ptr<EqualizerControllerListenerInterface> listener) {
    if (nullptr == listener) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullListener"));
        return;
    }

    std::lock_guard<std::mutex> guard(m_stateMutex);

    auto iter = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (m_listeners.end() != iter) {
        m_listeners.erase(iter);
    }
}

std::shared_ptr<EqualizerConfigurationInterface> EqualizerController::getConfiguration() {
    return m_configuration;
}

void EqualizerController::updateStateLocked() {
    std::string stateString = "mode=" + equalizerModeToString(m_currentState.mode);
    for (auto& bandIt : m_currentState.bandLevels) {
        EqualizerBand band = bandIt.first;
        int bandLevel = bandIt.second;
        stateString += "," + equalizerBandToString(band) + "=" + std::to_string(bandLevel);
    }
    ACSDK_DEBUG5(LX(__func__).d("new state", stateString));

    m_storage->saveState(m_currentState);

    for (const auto& equalizer : m_equalizers) {
        int maxLevel = equalizer->getMaximumBandLevel();
        int minLevel = equalizer->getMinimumBandLevel();
        if (maxLevel < m_configuration->getMaxBandLevel() || minLevel > m_configuration->getMinBandLevel()) {
            EqualizerBandLevelMap bandLevels;
            for (auto& it : m_currentState.bandLevels) {
                EqualizerBand band = it.first;
                int level = it.second;
                level = std::min(std::max(level, minLevel), maxLevel);
                bandLevels[band] = level;
            }
            equalizer->setEqualizerBandLevels(bandLevels);
        } else {
            equalizer->setEqualizerBandLevels(m_currentState.bandLevels);
        }
    }

    for (auto listener : m_listeners) {
        listener->onEqualizerStateChanged(m_currentState);
    }
}

EqualizerState EqualizerController::getCurrentState() {
    std::lock_guard<std::mutex> guard(m_stateMutex);

    return m_currentState;
}

}  // namespace equalizer
}  // namespace alexaClientSDK
