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
#include <ostream>

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "SoftwareComponentReporter/SoftwareComponentReporterCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace softwareComponentReporter {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
static const std::string TAG{"SoftwareComponentReporterCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// SoftwareComponentReporter capability constants
/// SoftwareComponentReporter interface type
static const std::string SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// SoftwareComponentReporter interface name
static const std::string SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_NAME = "Alexa.SoftwareComponentReporter";
/// SoftwareComponentReporter interface name
static const std::string SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// The softwareComponents key used in @c SoftwareComponentReporterCapabilityAgent configurations.
static const std::string SOFTWARECOMPONENTS_KEY = "softwareComponents";

/// The name key used by a component in SoftwareComponents.
static const std::string SOFTWARECOMPONENTS_NAME_KEY = "name";

/// The version key used by a component in SoftwareComponents.
static const std::string SOFTWARECOMPONENTS_VERSION_KEY = "version";

std::shared_ptr<SoftwareComponentReporterCapabilityAgent> SoftwareComponentReporterCapabilityAgent::create() {
    return std::shared_ptr<SoftwareComponentReporterCapabilityAgent>(new SoftwareComponentReporterCapabilityAgent());
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SoftwareComponentReporterCapabilityAgent::
    getCapabilityConfigurations() {
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> capabilityConfigurations;
    capabilityConfigurations.insert(buildCapabilityConfiguration());

    return capabilityConfigurations;
}

bool SoftwareComponentReporterCapabilityAgent::addConfiguration(
    const std::shared_ptr<avsCommon::avs::ComponentConfiguration> configuration) {
    if (!configuration) {
        ACSDK_ERROR(LX(__func__).m("configuration is null"));
        return false;
    }

    bool success = m_configurations.insert(configuration).second;
    if (!success) {
        ACSDK_ERROR(LX(__func__)
                        .d("name", configuration->name)
                        .d("version", configuration->version)
                        .m("component already exists"));
    }

    return success;
}

std::shared_ptr<avsCommon::avs::CapabilityConfiguration> SoftwareComponentReporterCapabilityAgent::
    buildCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SOFTWARECOMPONENTREPORTER_CAPABILITY_INTERFACE_VERSION});

    // generate softwareComponents array to add to configurations field of capability json
    if (!m_configurations.empty()) {
        JsonGenerator configurations;

        std::vector<std::string> softwareComponentsJsons;
        for (const auto& configuration : m_configurations) {
            JsonGenerator softwareComponentsJsonGenerator;
            softwareComponentsJsonGenerator.addMember(SOFTWARECOMPONENTS_NAME_KEY, configuration->name);
            softwareComponentsJsonGenerator.addMember(SOFTWARECOMPONENTS_VERSION_KEY, configuration->version);
            softwareComponentsJsons.push_back(softwareComponentsJsonGenerator.toString());
        }
        configurations.addMembersArray(SOFTWARECOMPONENTS_KEY, softwareComponentsJsons);

        configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, configurations.toString()});
    }

    return std::make_shared<avsCommon::avs::CapabilityConfiguration>(configMap);
}

}  // namespace softwareComponentReporter
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
