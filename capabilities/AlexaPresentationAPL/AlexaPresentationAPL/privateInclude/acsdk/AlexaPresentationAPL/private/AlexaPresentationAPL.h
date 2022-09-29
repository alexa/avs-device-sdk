/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPL_H_
#define ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPL_H_

#include <memory>
#include <string>

#include <acsdk/APLCapabilityCommonInterfaces/APLVideoConfiguration.h>
#include <acsdk/APLCapabilityCommon/BaseAPLCapabilityAgent.h>

#include "acsdk/AlexaPresentationAPL/private/AlexaPresentationAPLVideoConfigParser.h"

namespace alexaClientSDK {
namespace aplCapabilityAgent {
/**
 * This class implements a @c CapabilityAgent that handles the @c AlexaPresentationAPL API.  The
 * @c AlexaPresentationAPL is responsible for handling the directives with Alexa.Presentation.APL namespace.
 */
class AlexaPresentationAPL : public aplCapabilityCommon::BaseAPLCapabilityAgent {
public:
    /**
     * Create an instance of @c AlexaPresentationAPL.
     *
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface that sends exception messages to AVS.
     * @param metricRecorder The object @c MetricRecorderInterface that records metrics.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param APLVersion The APLVersion supported by the runtime component.
     * @param visualStateProvider The @c VisualStateProviderInterface used to request visual context.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c AlexaPresentationAPL.
     */
    static std::shared_ptr<AlexaPresentationAPL> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::string APLVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider = nullptr);

    /**
     * Destructor.
     */
    virtual ~AlexaPresentationAPL() = default;

    /// @name BaseAPLCapabilityAgent template functions
    /// @{
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getAPLDirectiveConfiguration() const override;
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getAPLCapabilityConfigurations(const std::string& APLMaxVersion) override;
    aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType getDirectiveType(
        std::shared_ptr<DirectiveInfo> info) override;
    const std::string& getConfigurationRootKey() override;
    const std::string& getMetricDataPointName(aplCapabilityCommon::BaseAPLCapabilityAgent::MetricEvent event) override;
    const std::string& getMetricActivityName(
        aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity activity) override;
    aplCapabilityCommon::BaseAPLCapabilityAgent::PresentationSessionFieldNames getPresentationSessionFieldNames()
        override;
    bool initialize() override;
    const bool shouldPackPresentationSessionToAvsEvents() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param exceptionSender The object to send AVS Exception messages.
     * @param metricRecorder The object to use for recording metrics.
     * @param messageSender The object to send message to AVS.
     * @param contextManager The object to fetch the context of the system.
     * @param visualStateProvider The VisualStateProviderInterface object used to request visual context.
     */
    AlexaPresentationAPL(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::string APLVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider = nullptr);

    /// Map for Metric event names
    static std::map<MetricEvent, std::string> METRICS_DATA_POINT_NAMES;
    /// Map for Metric Activity names
    static std::map<MetricActivity, std::string> METRICS_ACTIVITY_NAMES;

    /**
     * Creates the Alexa.Presentation.APL interface configuration.
     *
     * @return The Alexa.Presentation.APL interface configuration.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>
    getAlexaPresentationAPLCapabilityConfiguration(const std::string& APLMaxVersion);

    /**
     * Creates the Alexa.Presentation.Video interface configuration.
     *
     * @return The Alexa.Presentation.Video interface configuration.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>
    getAlexaPresentationVideoCapabilityConfiguration();

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
        m_capabilityConfigurations;

    /// Video settings to be reported for Alexa.Presentation.APL.Video interface
    alexaClientSDK::aplCapabilityCommonInterfaces::VideoSettings m_videoSettings;
};

}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPL_H_
