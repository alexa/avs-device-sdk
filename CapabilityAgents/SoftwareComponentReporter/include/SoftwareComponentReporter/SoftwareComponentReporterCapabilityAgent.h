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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SOFTWARECOMPONENTREPORTER_INCLUDE_SOFTWARECOMPONENTREPORTER_SOFTWARECOMPONENTREPORTERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SOFTWARECOMPONENTREPORTER_INCLUDE_SOFTWARECOMPONENTREPORTER_SOFTWARECOMPONENTREPORTERCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ComponentReporterInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace softwareComponentReporter {

/**
 * Capability Agent to collect the component configurations and report them to AVS.
 */
class SoftwareComponentReporterCapabilityAgent
        : public avsCommon::sdkInterfaces::ComponentReporterInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface {
public:
    /**
     * Destructor.
     */
    ~SoftwareComponentReporterCapabilityAgent() = default;

    /**
     * Create Method for Capability Agent
     *
     * @return Returns a shared_ptr to the SoftwareComponentReporterCapabilityAgent.
     */
    static std::shared_ptr<SoftwareComponentReporterCapabilityAgent> create();

    /// @name ComponentReporterInterface Functions
    /// @{
    bool addConfiguration(const std::shared_ptr<avsCommon::avs::ComponentConfiguration> configuration) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor.
     */
    SoftwareComponentReporterCapabilityAgent() = default;

    /**
     * Builds the capability configuration required for discovery.
     * Configuration will contain the configurations that have been added to the Capability Agent
     *
     * @return The @c CapabilityConfiguration of the Capability Agent.
     */
    std::shared_ptr<avsCommon::avs::CapabilityConfiguration> buildCapabilityConfiguration();

    // list of component configurations
    std::unordered_set<std::shared_ptr<avsCommon::avs::ComponentConfiguration>> m_configurations;
};

}  // namespace softwareComponentReporter
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SOFTWARECOMPONENTREPORTER_INCLUDE_SOFTWARECOMPONENTREPORTER_SOFTWARECOMPONENTREPORTERCAPABILITYAGENT_H_
