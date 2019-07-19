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

#ifndef ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_INMEMORYEQUALIZERCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_INMEMORYEQUALIZERCONFIGURATION_H_

#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerConfigurationInterface.h>

namespace alexaClientSDK {
namespace equalizer {

/**
 * In-memory implementation for the equalizer configuration. The configuration is set during the creation and is not
 * changed afterwards. This class also holds the hardcoded defaults used by SDK.
 */
class InMemoryEqualizerConfiguration : public avsCommon::sdkInterfaces::audio::EqualizerConfigurationInterface {
public:
    /**
     * Destructor.
     */
    ~InMemoryEqualizerConfiguration() override = default;

    /**
     * Factory to create an instance of @c InMemoryEqualizerConfiguration from the parameters provided.
     *
     * @param minBandLevel Minimum band level supported by the equalizer.
     * @param maxBandLevel Maximum band level supported by the equalizer.
     * @param bandsSupported A set of bands supported by the equalizer.
     * @param modesSupported A set of modes supported by the equalizer.
     * @param defaultState Default state of the equalizer used when there is no state stored in a persistent storage or
     * when a band level is being reset.
     * @return An instance of @c InMemoryEqualizerConfiguration on success, or @c nullptr in case of invalid parameters
     * or inconsistent configuration.
     */
    static std::shared_ptr<InMemoryEqualizerConfiguration> create(
        int minBandLevel,
        int maxBandLevel,
        const std::set<avsCommon::sdkInterfaces::audio::EqualizerBand>& bandsSupported,
        const std::set<avsCommon::sdkInterfaces::audio::EqualizerMode>& modesSupported,
        avsCommon::sdkInterfaces::audio::EqualizerState defaultState);

    /**
     * Factory to create disabled version of the configuration.
     *
     * @return An instance of disabled version of the configuration.
     */
    static std::shared_ptr<InMemoryEqualizerConfiguration> createDisabled();

    /**
     * A factory to create an instance of @c InMemoryEqualizerConfiguration using the hardcoded defaults.
     *
     * @return An instance of @c InMemoryEqualizerConfiguration using the hardcoded defaults.
     */
    static std::shared_ptr<InMemoryEqualizerConfiguration> createDefault();

    /// @name EqualizerConfigurationInterface functions
    /// @{
    bool isEnabled() const override;

    std::set<avsCommon::sdkInterfaces::audio::EqualizerBand> getSupportedBands() const override;

    std::set<avsCommon::sdkInterfaces::audio::EqualizerMode> getSupportedModes() const override;

    int getMinBandLevel() const override;

    int getMaxBandLevel() const override;

    avsCommon::sdkInterfaces::audio::EqualizerState getDefaultState() const override;

    bool isBandSupported(avsCommon::sdkInterfaces::audio::EqualizerBand band) const override;

    bool isModeSupported(avsCommon::sdkInterfaces::audio::EqualizerMode mode) const override;
    ///@}

protected:
    /**
     * Constructor.
     *
     * @param minBandLevel Minimum band level supported by the equalizer.
     * @param maxBandLevel Maximum band level supported by the equalizer.
     * @param bandsSupported A set of bands supported by the equalizer.
     * @param modesSupported A set of modes supported by the equalizer.
     * @param defaultState Default state of the equalizer used when there is no state stored in a persistent storage or
     * when a band level is being reset.
     */
    InMemoryEqualizerConfiguration(
        int minBandLevel,
        int maxBandLevel,
        const std::set<avsCommon::sdkInterfaces::audio::EqualizerBand>& bandsSupported,
        const std::set<avsCommon::sdkInterfaces::audio::EqualizerMode>& modesSupported,
        avsCommon::sdkInterfaces::audio::EqualizerState defaultState);

    /**
     * Constructor creating a disabled configuration.
     */
    InMemoryEqualizerConfiguration();

    /**
     * Validates the initialized configuration for consistency.
     *
     * @return True if configuration is consistent, false otherwise.
     */
    bool validateConfiguration();

    /**
     * Validates @c EqualizerBandLevelMap for consistency.
     *
     * @param bandLevelMap A @c EqualizerBandLevelMap to validate.
     * @param validateValues A flag indicating whether to check band levels for the allowed range. This could be false
     * for the case when allowed range is inconsistent and there is no point in validating against it.
     * @return True if all bands are supported and (optionally) band levels are withing the supported range, false
     * otherwise.
     */
    bool validateBandLevelMap(
        const avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap& bandLevelMap,
        bool validateValues);

private:
    /// Maximum supported level for an equalizer band.
    int m_maxBandLevel;

    /// Minimum supported level for an equalizer band.
    int m_minBandLevel;

    /// Set of bands supported.
    std::set<avsCommon::sdkInterfaces::audio::EqualizerBand> m_bandsSupported;

    /// Set of modes supported.
    std::set<avsCommon::sdkInterfaces::audio::EqualizerMode> m_modesSupported;

    /// Default equalizer state.
    avsCommon::sdkInterfaces::audio::EqualizerState m_defaultState;

    /// Flag indicating whether equalizer is enabled.
    bool m_isEnabled;
};

}  // namespace equalizer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_EQUALIZERIMPLEMENTATIONS_INCLUDE_EQUALIZERIMPLEMENTATIONS_INMEMORYEQUALIZERCONFIGURATION_H_
