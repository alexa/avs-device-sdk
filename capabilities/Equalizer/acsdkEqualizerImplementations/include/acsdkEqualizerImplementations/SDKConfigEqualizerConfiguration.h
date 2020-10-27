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

#ifndef ACSDKEQUALIZERIMPLEMENTATIONS_SDKCONFIGEQUALIZERCONFIGURATION_H_
#define ACSDKEQUALIZERIMPLEMENTATIONS_SDKCONFIGEQUALIZERCONFIGURATION_H_

#include <memory>
#include <mutex>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include "acsdkEqualizerImplementations/InMemoryEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace acsdkEqualizer {

/**
 * An implementation of @c EqualizerConfigurationInterface that uses SDK configuration to initialize.
 */
class SDKConfigEqualizerConfiguration : public InMemoryEqualizerConfiguration {
public:
    /**
     * Options controlling different aspects of SDK configuration interpretation.
     */

    /**
     * Flag indicating whether we should treat a band as supported by default if "bands" configuration branch exists in
     * JSON configuration.
     */
    static const bool BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG = false;
    /**
     * Flag indicating whether we should treat a mode as supported by default if "modes" configuration branch exists in
     * JSON configuration.
     */
    static const bool MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG = false;

    /**
     * Factory method to create an instance of @c EqualizerConfigurationInterface.
     *
     * @param configRoot @c ConfigurationNode containing configurations for the application (root-level).
     * @return A new instance of EqualizerConfigurationInterface on success or @c nullptr in case of failure.
     */
    static std::shared_ptr<EqualizerConfigurationInterface> createEqualizerConfigurationInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configRoot);

    /**
     * Factory method to create an instance of @c SDKConfigEqualizerConfiguration.
     *
     * @deprecated
     * @param configRoot @c ConfigurationNode containing the equalizer capabilities.
     * @return A new instance of SDKConfigEqualizerConfiguration on success or @c nullptr in case of failure.
     */
    static std::shared_ptr<SDKConfigEqualizerConfiguration> create(
        avsCommon::utils::configuration::ConfigurationNode& configRoot);

private:
    /**
     * Constructor.
     *
     * @param minBandLevel Minimum band level supported by the equalizer.
     * @param maxBandLevel Maximum band level supported by the equalizer.
     * @param defaultDelta Default delta value to adjust the equalizer.
     * @param bandsSupported A set of bands supported by the equalizer.
     * @param modesSupported A set of modes supported by the equalizer.
     * @param defaultState Default state of the equalizer used when there is no state stored in a persistent storage or
     * when a band level is being reset.
     */
    SDKConfigEqualizerConfiguration(
        int minBandLevel,
        int maxBandLevel,
        int defaultDelta,
        std::set<acsdkEqualizerInterfaces::EqualizerBand> bandsSupported,
        std::set<acsdkEqualizerInterfaces::EqualizerMode> modesSupported,
        acsdkEqualizerInterfaces::EqualizerState defaultState);

    /**
     * Constructor for disabled configuration.
     */
    SDKConfigEqualizerConfiguration();
};

}  // namespace acsdkEqualizer
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERIMPLEMENTATIONS_SDKCONFIGEQUALIZERCONFIGURATION_H_
