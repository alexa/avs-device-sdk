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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "EqualizerImplementations/SDKConfigEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::error;

/*
 * Example configuration
 *

"equalizer": {
    "enabled": true,
    "bands": {
        "BASS": true,
        "MIDRANGE": true,
        "TREBLE": false
    },
    "modes": {
        "NIGHT": true,
        "TV": true
    },
    "defaultState": {
        "mode": "NIGHT",
        "bands": {
            "BASS": 4,
            "MIDRANGE": -2
        }
    },
    "minLevel": -6,
    "maxLevel": 6
}

*/

/// String to identify log entries originating from this file.
static const std::string TAG{"SDKConfigEqualizerConfiguration"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// JSON key for "enabled" value.
static const std::string ENABLED_CONFIG_KEY = "enabled";
/// JSON key for "equalizer" branch.
static const std::string BANDS_CONFIG_KEY = "bands";
/// JSON key for "modes" branch.
static const std::string MODES_CONFIG_KEY = "modes";
/// JSON key for "mode" value.
static const std::string MODE_CONFIG_KEY = "mode";
/// JSON key for "defaultState" branch.
static const std::string DEFAULT_STATE_CONFIG_KEY = "defaultState";
/// JSON key for "minLevel" value.
static const std::string MIN_LEVEL_CONFIG_KEY = "minLevel";
/// JSON key for "maxLevel" value.
static const std::string MAX_LEVEL_CONFIG_KEY = "maxLevel";

const bool SDKConfigEqualizerConfiguration::BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG;
const bool SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG;

std::shared_ptr<SDKConfigEqualizerConfiguration> SDKConfigEqualizerConfiguration::create(
    ConfigurationNode& configRoot) {
    std::set<EqualizerBand> bandsSupported;
    std::set<EqualizerMode> modesSupported;
    EqualizerState defaultState;
    bool hasErrors = false;

    bool isEnabled;
    configRoot.getBool(ENABLED_CONFIG_KEY, &isEnabled, true);

    if (!isEnabled) {
        return std::shared_ptr<SDKConfigEqualizerConfiguration>(new SDKConfigEqualizerConfiguration());
    }

    auto defaultConfiguration = InMemoryEqualizerConfiguration::createDefault();

    int minLevel;
    int maxLevel;

    configRoot.getInt(MIN_LEVEL_CONFIG_KEY, &minLevel, defaultConfiguration->getMinBandLevel());
    configRoot.getInt(MAX_LEVEL_CONFIG_KEY, &maxLevel, defaultConfiguration->getMaxBandLevel());

    auto supportedBandsBranch = configRoot[BANDS_CONFIG_KEY];
    if (supportedBandsBranch) {
        for (EqualizerBand band : EqualizerBandValues) {
            std::string bandName = equalizerBandToString(band);
            bool value;
            supportedBandsBranch.getBool(bandName, &value, BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
            if (value) {
                bandsSupported.insert(band);
            }
        }
    } else {
        // Using default bands
        bandsSupported = defaultConfiguration->getSupportedBands();
    }

    auto supportedModesBranch = configRoot[MODES_CONFIG_KEY];
    if (supportedModesBranch) {
        for (EqualizerMode mode : EqualizerModeValues) {
            if (EqualizerMode::NONE == mode) {
                continue;
            }
            std::string modeName = equalizerModeToString(mode);

            bool value;
            supportedModesBranch.getBool(modeName, &value, MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
            if (value) {
                modesSupported.insert(mode);
            }
        }
    } else {
        // Using default modes
        modesSupported = defaultConfiguration->getSupportedModes();
    }

    bool hasDefaultStateDefined = false;

    EqualizerState defaultConfigDefaultState = defaultConfiguration->getDefaultState();
    defaultState.mode = defaultConfigDefaultState.mode;
    auto defaultStateBranch = configRoot[DEFAULT_STATE_CONFIG_KEY];
    if (defaultStateBranch) {
        std::string defaultModeStr;
        if (defaultStateBranch.getString(MODE_CONFIG_KEY, &defaultModeStr)) {
            SuccessResult<EqualizerMode> modeResult = stringToEqualizerMode(defaultModeStr);

            if (!modeResult.isSucceeded()) {
                ACSDK_WARN(LX(__func__)
                               .m("Invalid value for default state mode, assuming none set")
                               .d("value", defaultModeStr));
            } else {
                EqualizerMode defaultMode = modeResult.value();
                // Check if mode is supported
                if (modesSupported.end() == modesSupported.find(defaultMode)) {
                    ACSDK_ERROR(LX("createFailed")
                                    .d("reason", "Unsupported mode is set as default state mode")
                                    .d("mode", defaultModeStr));
                    hasErrors = true;
                } else {
                    defaultState.mode = defaultMode;
                }
            }
        }  // has "modes" key

        // Parse bands
        auto defaultBandsBranch = defaultStateBranch[BANDS_CONFIG_KEY];
        if (defaultBandsBranch) {
            EqualizerBandLevelMap levelMap;

            for (EqualizerBand band : bandsSupported) {
                std::string bandName = equalizerBandToString(band);
                int level;
                if (!defaultBandsBranch.getInt(bandName, &level)) {
                    ACSDK_ERROR(LX("createFailed")
                                    .d("reason", "Default state definition incomplete")
                                    .d("missing band", bandName));
                    hasErrors = true;
                }
                levelMap[band] = level;
            }
            defaultState.bandLevels = levelMap;
            hasDefaultStateDefined = true;
        }
    }
    if (!hasDefaultStateDefined) {  // If defaultStateBranch
        // Use only supported bands from the default state.
        for (auto band : bandsSupported) {
            defaultState.bandLevels[band] = defaultConfigDefaultState.bandLevels[band];
        }
    }

    if (hasErrors) {
        return nullptr;
    }

    // Initialize configuration
    auto config = std::shared_ptr<SDKConfigEqualizerConfiguration>(
        new SDKConfigEqualizerConfiguration(minLevel, maxLevel, bandsSupported, modesSupported, defaultState));

    if (!config->validateConfiguration()) {
        // Validation messages are already in logs.
        return nullptr;
    }

    // Configuration is valid but let's check some unusual setup.
    if (bandsSupported.empty()) {
        ACSDK_WARN(LX(__func__).m("Configuration has no bands supported while Equalizer is enabled. Is it intended?"));
    }

    return config;
}

SDKConfigEqualizerConfiguration::SDKConfigEqualizerConfiguration(
    int minBandLevel,
    int maxBandLevel,
    std::set<EqualizerBand> bandsSupported,
    std::set<avsCommon::sdkInterfaces::audio::EqualizerMode> modesSupported,
    EqualizerState defaultState) :
        InMemoryEqualizerConfiguration(minBandLevel, maxBandLevel, bandsSupported, modesSupported, defaultState) {
}

SDKConfigEqualizerConfiguration::SDKConfigEqualizerConfiguration() : InMemoryEqualizerConfiguration() {
}

}  // namespace equalizer
}  // namespace alexaClientSDK
