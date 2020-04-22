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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_STATEREPORTGENERATOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_STATEREPORTGENERATOR_H_

#include <array>
#include <memory>
#include <string>

#include <AVSCommon/Utils/PlatformDefinitions.h>
#include <AVSCommon/Utils/Optional.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingStringConversion.h>
#include <Settings/SettingsManagerBuilderBase.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class is responsible for generating a state report for every setting available in a given @c SettingsManager.
 *
 * @note This class is not thread safe.
 */
class StateReportGenerator {
public:
    /// Alias for the @c SettingsConfigurations of a given @c SettingsManager.
    /// @tparam SettingsManagerT The target settings manager type.
    template <class SettingsManagerT>
    using SettingConfigurations =
        typename settings::SettingsManagerBuilderBase<SettingsManagerT>::SettingConfigurations;

    /**
     * Create a StateReportGenerator for the given manager with the provided configurations.
     *
     * @tparam SettingsManagerT The type of the SettingManager.
     * @param settingManager The manager that exposes the setting states.
     * @param configurations The configurations which hold the metadata to generate the report.
     * @return An optional with a valid StateReportGenerator if it succeeds; an empty optional otherwise.
     */
    template <class SettingsManagerT>
    static avsCommon::utils::Optional<StateReportGenerator> create(
        std::shared_ptr<SettingsManagerT> settingManager,
        const SettingConfigurations<SettingsManagerT>& configurations);

    /**
     * Generate a report for each setting.
     *
     * @return The generated reports.
     */
    std::vector<std::string> generateReport();

    /**
     * Default constructor. This should only be used by optional.
     */
    StateReportGenerator() = default;

protected:
    /**
     * Constructor (protected for unit tests).
     *
     * @param reportFunctions The collection of report functions used to generate the entire report.
     */
    explicit StateReportGenerator(const std::vector<std::function<std::string()>>& reportFunctions);

private:
    /**
     * Utility structure used to wrap each setting into a report generation function.
     *
     * @tparam SettingsManagerT The type of the settings manager.
     * @tparam index The index of the setting being wrapped.
     */
    template <class SettingsManagerT, ssize_t index = SettingsManagerT::NUMBER_OF_SETTINGS - 1>
    struct StringFunctionWrapper {
        /**
         * The operator() which will subscribe the generation function to @c reportFunctions.
         *
         * @param manager The target setting manager.
         * @param configurations The settings configuration for the setting manager.
         * @param reportFunctions Collection of report functions.
         */
        void operator()(
            const std::shared_ptr<SettingsManagerT>& manager,
            const SettingConfigurations<SettingsManagerT>& configurations,
            std::vector<std::function<std::string()>>& reportFunctions);
    };

    /**
     * Specialization of the @c StringFunctionWrapper which will stop the recursive setting addition.
     *
     * @tparam SettingsManagerT The type of the settings manager.
     */
    template <class SettingsManagerT>
    struct StringFunctionWrapper<SettingsManagerT, -1> {
        /**
         * The operator() which will subscribe the generation function to @c reportFunctions.
         *
         * @param manager The target setting manager.
         * @param configurations The settings configuration for the setting manager.
         * @param reportFunctions Collection of report functions.
         */
        void operator()(
            const std::shared_ptr<SettingsManagerT>& manager,
            const SettingConfigurations<SettingsManagerT>& configurations,
            std::vector<std::function<std::string()>>& reportFunctions);
    };

    /**
     * The function that generates the report of one setting.
     *
     * @param metadata The metadata with the information need to generate the report.
     * @param value The current value of the setting.
     * @return The state report of a given setting.
     */
    static std::string generateSettingStateReport(
        const settings::SettingEventMetadata& metadata,
        const std::string& value);

    /// The report functions.
    std::vector<std::function<std::string()>> m_reportFunctions;
};

template <class SettingsManagerT>
avsCommon::utils::Optional<StateReportGenerator> StateReportGenerator::create(
    std::shared_ptr<SettingsManagerT> manager,
    const SettingConfigurations<SettingsManagerT>& configurations) {
    if (!manager) {
        avsCommon::utils::logger::acsdkError(
            avsCommon::utils::logger::LogEntry("StateReportGenerator", "createFailed").d("reason", "nullManager"));
        return avsCommon::utils::Optional<StateReportGenerator>();
    }
    std::vector<std::function<std::string()>> reportFunctions;
    StringFunctionWrapper<SettingsManagerT> wrapper;
    wrapper(manager, configurations, reportFunctions);
    return avsCommon::utils::Optional<StateReportGenerator>(StateReportGenerator{reportFunctions});
}

template <class SettingsManagerT, ssize_t index>
void StateReportGenerator::StringFunctionWrapper<SettingsManagerT, index>::operator()(
    const std::shared_ptr<SettingsManagerT>& settingsManager,
    const SettingConfigurations<SettingsManagerT>& configurations,
    std::vector<std::function<std::string()>>& reportFunctions) {
    if (std::get<index>(configurations).metadata.hasValue()) {
        auto metadata = std::get<index>(configurations).metadata.value();
        reportFunctions.push_back([settingsManager, metadata] {
            return generateSettingStateReport(metadata, settingsManager->template getJsonValue<index>());
        });
    }

    StringFunctionWrapper<SettingsManagerT, index - 1> wrapper;
    wrapper(settingsManager, configurations, reportFunctions);
}

template <class SettingsManagerT>
void StateReportGenerator::StringFunctionWrapper<SettingsManagerT, -1>::operator()(
    const std::shared_ptr<SettingsManagerT>& settingsManager,
    const SettingConfigurations<SettingsManagerT>& configurations,
    std::vector<std::function<std::string()>>& reportFunctions) {
    // Do nothing. This is the end of the loop.
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_STATEREPORTGENERATOR_H_
