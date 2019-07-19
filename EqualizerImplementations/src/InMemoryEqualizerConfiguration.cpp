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

#include <unordered_set>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <EqualizerImplementations/InMemoryEqualizerConfiguration.h>

#include "EqualizerImplementations/InMemoryEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::sdkInterfaces::audio;

/// String to identify log entries originating from this file.
static const std::string TAG{"InMemoryEqualizerConfiguration"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Default level value.
static constexpr int DEFAULT_LEVEL = 0;
/// Default minimum band level in dB. -6 dB effectively halving the amplitude of the band.
static constexpr int DEFAULT_MIN_LEVEL = -6;
/// Default maximum band level in dB. +6 dB effectively doubling the amplitude of the band.
static constexpr int DEFAULT_MAX_LEVEL = 6;

std::set<EqualizerBand> InMemoryEqualizerConfiguration::getSupportedBands() const {
    return m_bandsSupported;
}

std::set<EqualizerMode> InMemoryEqualizerConfiguration::getSupportedModes() const {
    return m_modesSupported;
}

int InMemoryEqualizerConfiguration::getMinBandLevel() const {
    return m_minBandLevel;
}

int InMemoryEqualizerConfiguration::getMaxBandLevel() const {
    return m_maxBandLevel;
}

EqualizerState InMemoryEqualizerConfiguration::getDefaultState() const {
    return m_defaultState;
}

bool InMemoryEqualizerConfiguration::isBandSupported(EqualizerBand band) const {
    if (!m_isEnabled) {
        return false;
    }
    return m_bandsSupported.end() != m_bandsSupported.find(band);
}

bool InMemoryEqualizerConfiguration::isModeSupported(EqualizerMode mode) const {
    if (!m_isEnabled) {
        return false;
    }
    return m_modesSupported.end() != m_modesSupported.find(mode);
}

InMemoryEqualizerConfiguration::InMemoryEqualizerConfiguration(
    int minBandLevel,
    int maxBandLevel,
    const std::set<EqualizerBand>& bandsSupported,
    const std::set<EqualizerMode>& modesSupported,
    EqualizerState defaultState) :
        m_maxBandLevel{maxBandLevel},
        m_minBandLevel{minBandLevel},
        m_bandsSupported{bandsSupported},
        m_modesSupported{modesSupported},
        m_defaultState(defaultState),
        m_isEnabled{true} {
    // Remove NONE mode if any
    m_modesSupported.erase(EqualizerMode::NONE);
}

bool InMemoryEqualizerConfiguration::validateConfiguration() {
    ACSDK_DEBUG1(LX(__func__).m("Validating Equalizer configuration"));
    if (!m_isEnabled) {
        return false;
    }

    bool isValid = true;
    bool areBandExtremumsValid = true;
    if (m_maxBandLevel < m_minBandLevel) {
        ACSDK_ERROR(LX("validateConfigurationFailed")
                        .d("reason", "Maximum band level must be greater than minimum band level")
                        .d("maxLevel", m_maxBandLevel)
                        .d("minLevel", m_minBandLevel));
        areBandExtremumsValid = false;
        isValid = false;
    }

    ACSDK_DEBUG1(LX(__func__).m("Validating default Equalizer state"));
    // Showing mode as NONE here to separate default values from normal modes
    isValid = validateBandLevelMap(m_defaultState.bandLevels, areBandExtremumsValid) && isValid;

    // Check default state
    if (EqualizerMode::NONE != m_defaultState.mode) {
        if (!isModeSupported(m_defaultState.mode)) {
            ACSDK_ERROR(LX("validateConfigurationFailed")
                            .d("reason", "Default mode is unsupported")
                            .d("mode", equalizerModeToString(m_defaultState.mode)));
            isValid = false;
        }
    }

    return isValid;
}

bool InMemoryEqualizerConfiguration::validateBandLevelMap(
    const EqualizerBandLevelMap& bandLevelMap,
    bool validateValues) {
    if (!m_isEnabled) {
        return false;
    }
    bool isValid = true;
    for (auto bandMapIter : bandLevelMap) {
        EqualizerBand band = bandMapIter.first;
        if (!isBandSupported(band)) {
            ACSDK_ERROR(LX("validateBandLevelMapFailed")
                            .d("reason", "Band unsupported")
                            .d("band", equalizerBandToString(band)));
            isValid = false;
        } else {
            // Check band values only if extremums are valid
            if (validateValues) {
                int bandLevel = bandMapIter.second;
                if (bandLevel < m_minBandLevel || bandLevel > m_maxBandLevel) {
                    ACSDK_ERROR(LX("validateBandLevelMapFailed")
                                    .d("reason", "Invalid level value")
                                    .d("level", bandLevel)
                                    .d("minimum", m_minBandLevel)
                                    .d("maximum", m_maxBandLevel)
                                    .d("band", equalizerBandToString(band)));
                    isValid = false;
                }
            }
        }
    }

    return isValid;
}

std::shared_ptr<InMemoryEqualizerConfiguration> InMemoryEqualizerConfiguration::create(
    int minBandLevel,
    int maxBandLevel,
    const std::set<EqualizerBand>& bandsSupported,
    const std::set<EqualizerMode>& modesSupported,
    EqualizerState defaultState) {
    auto configuration = std::shared_ptr<InMemoryEqualizerConfiguration>(
        new InMemoryEqualizerConfiguration(minBandLevel, maxBandLevel, bandsSupported, modesSupported, defaultState));
    if (!configuration->validateConfiguration()) {
        // Errors have already been logged by this point
        return nullptr;
    }

    return configuration;
}
std::shared_ptr<InMemoryEqualizerConfiguration> InMemoryEqualizerConfiguration::createDefault() {
    return create(
        DEFAULT_MIN_LEVEL,
        DEFAULT_MAX_LEVEL,
        std::set<EqualizerBand>({EqualizerBand::BASS, EqualizerBand::MIDRANGE, EqualizerBand::TREBLE}),
        std::set<EqualizerMode>(),
        EqualizerState{EqualizerMode::NONE,
                       EqualizerBandLevelMap({{EqualizerBand::BASS, DEFAULT_LEVEL},
                                              {EqualizerBand::MIDRANGE, DEFAULT_LEVEL},
                                              {EqualizerBand::TREBLE, DEFAULT_LEVEL}})});
}

bool InMemoryEqualizerConfiguration::isEnabled() const {
    return m_isEnabled;
}

std::shared_ptr<InMemoryEqualizerConfiguration> InMemoryEqualizerConfiguration::createDisabled() {
    return std::shared_ptr<InMemoryEqualizerConfiguration>(new InMemoryEqualizerConfiguration());
}

InMemoryEqualizerConfiguration::InMemoryEqualizerConfiguration() :
        m_maxBandLevel{DEFAULT_LEVEL},
        m_minBandLevel{DEFAULT_LEVEL},
        m_defaultState{EqualizerMode::NONE, {{}}},
        m_isEnabled{false} {
}

}  // namespace equalizer
}  // namespace alexaClientSDK
