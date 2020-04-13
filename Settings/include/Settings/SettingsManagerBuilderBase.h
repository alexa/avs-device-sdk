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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGERBUILDERBASE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGERBUILDERBASE_H_

#include <tuple>

#include <AVSCommon/Utils/Optional.h>

#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingsManager.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Structure to save a specific setting and its configuration.
 *
 * @tparam SettingsT The type of the setting.
 */
template <typename SettingsT>
struct SettingConfiguration {
    /// The setting configured.
    std::shared_ptr<SettingsT> setting;
    /// The setting metadata.
    avsCommon::utils::Optional<settings::SettingEventMetadata> metadata;
};

/**
 * Base template for settings manager builders.
 *
 * This definition is only needed to allow us to extract the parameter pack of a specific @c SettingsManager type.
 * @tparam ManagerT The type of the @c SettingsManager to be built.
 */
template <class ManagerT>
class SettingsManagerBuilderBase {};

/**
 * Base class for @c SettingManagers builders.
 *
 * @tparam SettingsT The types handled by the target @c SettingManagers.
 */
template <typename... SettingsT>
class SettingsManagerBuilderBase<settings::SettingsManager<SettingsT...>> {
public:
    /// The setting type kept at @c index position.
    template <size_t index>
    using SettingType = typename std::tuple_element<index, std::tuple<SettingsT...>>::type;

    /// The setting value type kept at @c index position.
    template <size_t index>
    using ValueType = typename SettingType<index>::ValueType;

    /// The tuple holding the settings configuration.
    using SettingConfigurations = std::tuple<SettingConfiguration<SettingsT>...>;

    /// The number of settings supported by this builder.
    static constexpr size_t NUMBER_OF_SETTINGS{sizeof...(SettingsT)};

    /**
     * Builds a @c SettingsManager object.
     *
     * @return A valid pointer if the build succeeds; @c false otherwise.
     */
    virtual std::unique_ptr<settings::SettingsManager<SettingsT...>> build() = 0;

    /**
     * Gets the settings configuration.
     *
     * @return the settings configuration.
     */
    const SettingConfigurations getConfigurations() const;

    /**
     * Virtual destructor.
     */
    virtual ~SettingsManagerBuilderBase() = default;

protected:
    /// A tuple with all setting configurations.
    SettingConfigurations m_settingConfigs;
};

template <typename... SettingsT>
const std::tuple<SettingConfiguration<SettingsT>...> SettingsManagerBuilderBase<
    SettingsManager<SettingsT...>>::getConfigurations() const {
    return m_settingConfigs;
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSMANAGERBUILDERBASE_H_
