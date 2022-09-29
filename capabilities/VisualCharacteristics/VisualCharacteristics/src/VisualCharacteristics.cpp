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

#include "acsdk/VisualCharacteristics/private/VisualCharacteristics.h"

#include <algorithm>
#include <unordered_map>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "acsdk/VisualCharacteristics/private/VCConfigParser.h"

namespace alexaClientSDK {
namespace visualCharacteristics {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace visualCharacteristicsInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"VisualCharacteristics"};

/// The key in our config file to find the root of VisualCharacteristics configuration
static const std::string VISUALCHARACTERISTICS_CONFIGURATION_ROOT_KEY = "visualCharacteristics";

/// The key in our config file to find the name of configuration node
static const std::string INTERFACE_CONFIGURATION_NAME_KEY = "interface";

/// The key in our config file to find the configurations of configuration node
static const std::string INTERFACE_CONFIGURATION_KEY = "configurations";

static const std::string INTERACTION_MODES = "interactionModes";

static const std::string TEMPLATES = "templates";

/// The default interface name if it's not present
static const std::string DEFAULT_INTERFACE_NAME = "";

/// Capability interface type
static const std::string CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// Alexa.InteractionMode interface name
static const std::string ALEXAINTERACTIONMODE_CAPABILITY_INTERFACE_NAME = "Alexa.InteractionMode";
/// Alexa.InteractionMode interface version
static const std::string ALEXAINTERACTIONMODE_CAPABILITY_INTERFACE_VERSION = "1.1";

/// Alexa.Display.Window interface name
static const std::string ALEXADISPLAYWINDOW_CAPABILITY_INTERFACE_NAME = "Alexa.Display.Window";
/// Alexa.Display.Window interface version
static const std::string ALEXADISPLAYWINDOW_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Alexa.Display interface name
static const std::string ALEXADISPLAY_CAPABILITY_INTERFACE_NAME = "Alexa.Display";
/// Alexa.Display interface version
static const std::string ALEXADISPLAY_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Namespace three supported by Alexa presentation APL capability agent.
static const std::string ALEXA_DISPLAY_WINDOW_NAMESPACE{"Alexa.Display.Window"};

/// Placeholder namespace used for exposing @c VisualCharacteristics via @c EndpointCapabilitiesBuilderInterface.
/// Note that this namespace is not used, but is required until VisualCharacteristics becomes part of SDK Core.
/// VisualCharacteristics is added using the ExternalCapabilityBuilderInterface prior to that time.
static const std::string PLACEHOLDER_CAPABILITY_NAMESPACE{"VisualCharacteristics"};

/// Tag for finding the device window state context information sent from the runtime as part of event context.
static const std::string WINDOW_STATE_NAME{"WindowState"};

/// Key for default window id for window state for Alexa.Display.Window.
static const std::string DEFAULT_WINDOW_ID{"defaultWindowId"};

/// Key for window id id for window state for Alexa.Display.Window.
static const std::string ID{"id"};

/// Key for template id id for window state for Alexa.Display.Window.
static const std::string TEMPLATE_ID{"templateId"};

/// Key for token id for window state for Alexa.Display.Window.
static const std::string TOKEN{"token"};

/// Key for interaction mode for window state for Alexa.Display.Window.
static const std::string INTERACTION_MODE{"interactionMode"};

/// Key for size configuration id for window state for Alexa.Display.Window.
static const std::string SIZE_CONFIGURATION_ID{"sizeConfigurationId"};

/// Key for configuration for window state for Alexa.Display.Window.
static const std::string CONFIGURATION{"configuration"};

/// Key for instance for window state for Alexa.Display.Window.
static const std::string INSTANCES{"instances"};

/// The VisualCharacteristics context state signature.
static const avsCommon::avs::NamespaceAndName DEVICE_WINDOW_STATE{ALEXA_DISPLAY_WINDOW_NAMESPACE, WINDOW_STATE_NAME};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<VisualCharacteristics> VisualCharacteristics::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender) {
    VisualCharacteristicsConfiguration configuration;
    if (!getVisualCharacteristicsCapabilityConfiguration(configuration)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Unable to retrieve capability configuration"));
        return nullptr;
    }

    return createWithConfiguration(contextManager, exceptionSender, configuration);
}

std::shared_ptr<VisualCharacteristics> VisualCharacteristics::createWithConfiguration(
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender,
    const VisualCharacteristicsConfiguration& configuration) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }

    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    if (!validateConfiguration(configuration)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Invalid configuration"));
        return nullptr;
    }

    std::shared_ptr<VisualCharacteristics> visualCharacteristics(
        new VisualCharacteristics(contextManager, exceptionSender, configuration));
    contextManager->setStateProvider(DEVICE_WINDOW_STATE, visualCharacteristics);
    return visualCharacteristics;
}

void VisualCharacteristics::doShutdown() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor->shutdown();
    m_contextManager.reset();
}

VisualCharacteristics::VisualCharacteristics(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender,
    const VisualCharacteristicsConfiguration& configuration) :
        CapabilityAgent{PLACEHOLDER_CAPABILITY_NAMESPACE, exceptionSender},
        RequiresShutdown{"VisualCharacteristics"},
        m_contextManager{std::move(contextManager)},
        m_visualCharacteristicsConfiguration(configuration) {
    m_executor = std::make_shared<avsCommon::utils::threading::Executor>();
    initializeCapabilityConfiguration();
}

DirectiveHandlerConfiguration VisualCharacteristics::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    return configuration;
}

bool VisualCharacteristics::getVisualCharacteristicsCapabilityConfiguration(
    VisualCharacteristicsConfiguration& configuration) {
    ACSDK_DEBUG9(LX(__func__));

    /// Get the root ConfigurationNode.
    auto configurationRoot = ConfigurationNode::getRoot();

    /// Get the ConfigurationNode contains VisualCharacteristics config array.
    auto configurationArray = configurationRoot.getArray(VISUALCHARACTERISTICS_CONFIGURATION_ROOT_KEY);

    /// Loop through the configuration node array and construct config for these APIs.
    for (size_t i = 0; i < configurationArray.getArraySize(); i++) {
        std::string interfaceName;
        configurationArray[i].getString(INTERFACE_CONFIGURATION_NAME_KEY, &interfaceName, DEFAULT_INTERFACE_NAME);

        if (ALEXAINTERACTIONMODE_CAPABILITY_INTERFACE_NAME == interfaceName) {
            auto interactionModes = configurationArray[i][INTERFACE_CONFIGURATION_KEY].getArray(INTERACTION_MODES);
            for (size_t j = 0; j < interactionModes.getArraySize(); ++j) {
                InteractionMode interactionMode;
                if (!VCConfigParser::parseInteractionMode(interactionModes[j], interactionMode)) {
                    ACSDK_ERROR(LX("getVisualCharacteristicsCapabilityConfigurationFailed")
                                    .d("reason", "Unable to retrieve interaction mode")
                                    .d("index", j));
                    return false;
                }
                configuration.interactionModes.push_back(interactionMode);
            }
        } else if (ALEXADISPLAYWINDOW_CAPABILITY_INTERFACE_NAME == interfaceName) {
            auto templates = configurationArray[i][INTERFACE_CONFIGURATION_KEY].getArray(TEMPLATES);
            for (size_t j = 0; j < templates.getArraySize(); ++j) {
                WindowTemplate windowTemplate;
                if (!VCConfigParser::parseWindowTemplate(templates[j], windowTemplate)) {
                    ACSDK_ERROR(LX("getVisualCharacteristicsCapabilityConfigurationFailed")
                                    .d("reason", "Unable to retrieve window template")
                                    .d("index", j));
                    return false;
                }
                configuration.windowTemplates.push_back(windowTemplate);
            }
        } else if (ALEXADISPLAY_CAPABILITY_INTERFACE_NAME == interfaceName) {
            if (!VCConfigParser::parseDisplayCharacteristics(
                    configurationArray[i][INTERFACE_CONFIGURATION_KEY], configuration.displayCharacteristics)) {
                ACSDK_ERROR(LX("getVisualCharacteristicsCapabilityConfigurationFailed")
                                .d("reason", "Unable to retrieve display characteristics"));
                return false;
            }
        }
    }

    return true;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> VisualCharacteristics::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void VisualCharacteristics::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    m_executor->submit([this, stateRequestToken] {
        std::string windowStateCtx;
        generateWindowStateContext(windowStateCtx);
        m_contextManager->setState(DEVICE_WINDOW_STATE, windowStateCtx, StateRefreshPolicy::ALWAYS, stateRequestToken);
    });
}

std::vector<WindowTemplate> VisualCharacteristics::getWindowTemplates() {
    return m_visualCharacteristicsConfiguration.windowTemplates;
}

std::vector<InteractionMode> VisualCharacteristics::getInteractionModes() {
    return m_visualCharacteristicsConfiguration.interactionModes;
}

DisplayCharacteristics VisualCharacteristics::getDisplayCharacteristics() {
    return m_visualCharacteristicsConfiguration.displayCharacteristics;
}

void VisualCharacteristics::setWindowInstances(
    const std::vector<WindowInstance>& instances,
    const std::string& defaultWindowInstanceId) {
    m_executor->submit([this, instances, defaultWindowInstanceId] {
        m_windowInstances.clear();
        m_tokenPerWindow.clear();
        for (const auto& instance : instances) {
            m_windowInstances[instance.id] = instance;
            if (instance.id == defaultWindowInstanceId) {
                m_defaultWindowId = instance.id;
            }
        }
    });
}

bool VisualCharacteristics::addWindowInstance(const WindowInstance& instance) {
    return m_executor
        ->submit([this, instance] {
            bool retVal = true;
            auto windowId = instance.id;
            if (m_windowInstances.find(windowId) == m_windowInstances.end()) {
                m_windowInstances[windowId] = instance;
            } else {
                ACSDK_ERROR(LX("addWindowInstance").d("reason", "duplicateInstance"));
                retVal = false;
            }

            return retVal;
        })
        .get();
}

bool VisualCharacteristics::removeWindowInstance(const std::string& windowInstanceId) {
    return m_executor
        ->submit([this, windowInstanceId] {
            bool retVal = true;
            auto it = m_windowInstances.find(windowInstanceId);
            if (it != m_windowInstances.end()) {
                m_windowInstances.erase(it);
                auto windowIdTokenIt = m_tokenPerWindow.find(windowInstanceId);
                if (windowIdTokenIt != m_tokenPerWindow.end()) {
                    m_tokenPerWindow.erase(windowIdTokenIt);
                }
            } else {
                ACSDK_ERROR(LX("removeWindowInstance").d("reason", "windowIdNotFound"));
                retVal = false;
            }

            return retVal;
        })
        .get();
}

void VisualCharacteristics::updateWindowInstance(const WindowInstance& instance) {
    m_executor->submit([this, instance] {
        bool retVal = true;
        auto windowId = instance.id;
        auto it = m_windowInstances.find(windowId);
        if (it != m_windowInstances.end()) {
            it->second = instance;
        } else {
            ACSDK_ERROR(LX("updateWindowInstance").d("reason", "windowIdNotFound"));
            retVal = false;
        }

        return retVal;
    });
}

bool VisualCharacteristics::setDefaultWindowInstance(const std::string& windowInstanceId) {
    ACSDK_DEBUG0(LX(__func__).d("windowInstanceId", windowInstanceId));
    return m_executor
        ->submit([this, windowInstanceId] {
            bool retVal = true;

            if (m_windowInstances.find(windowInstanceId) != m_windowInstances.end()) {
                m_defaultWindowId = windowInstanceId;
            } else {
                ACSDK_ERROR(LX("setDefaultWindowInstance").d("reason", "windowIdNotFound"));
                retVal = false;
            }

            return retVal;
        })
        .get();
}

void VisualCharacteristics::onStateChanged(
    const std::string& windowId,
    const presentationOrchestratorInterfaces::PresentationMetadata& metadata) {
    m_executor->submit([this, windowId, metadata] {
        auto it = m_windowInstances.find(windowId);
        if (it != m_windowInstances.end()) {
            m_tokenPerWindow[windowId] = metadata.metadata;
        }
    });
}

void VisualCharacteristics::setExecutor(const std::shared_ptr<avsCommon::utils::threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).m("should be called in test only"));
    m_executor = executor;
}

bool VisualCharacteristics::validateConfiguration(const VisualCharacteristicsConfiguration& configuration) {
    ACSDK_DEBUG9(LX(__func__)
                     .d("length", configuration.interactionModes.size())
                     .d("isEmpty", configuration.interactionModes.empty()));
    if (configuration.interactionModes.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing interaction modes"));
        return false;
    }

    if (configuration.windowTemplates.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing window templates"));
        return false;
    }

    // Ensure that all interaction modes in each window template match to any of the defined interaction modes
    for (auto& windowTemplate : configuration.windowTemplates) {
        auto present = std::all_of(
            windowTemplate.interactionModes.begin(),
            windowTemplate.interactionModes.end(),
            [&](std::string const& modeName) {
                return std::any_of(
                    configuration.interactionModes.begin(),
                    configuration.interactionModes.end(),
                    [&modeName](InteractionMode const& interactionMode) { return interactionMode.id == modeName; });
            });

        if (!present) {
            ACSDK_ERROR(LX(__func__).d("reason", "Interaction mode not found"));
            return false;
        }
    }

    return true;
}

void VisualCharacteristics::generateWindowStateContext(std::string& stateJson) {
    rapidjson::Document configurationsJson(rapidjson::kObjectType);
    auto& alloc = configurationsJson.GetAllocator();
    rapidjson::Value payload(rapidjson::kObjectType);
    payload.AddMember(rapidjson::StringRef(DEFAULT_WINDOW_ID), m_defaultWindowId, alloc);

    rapidjson::Value instancesJson(rapidjson::kArrayType);
    for (auto& windowIdAndInstance : m_windowInstances) {
        auto& windowId = windowIdAndInstance.first;
        auto& windowInstance = windowIdAndInstance.second;
        rapidjson::Value instanceJson(rapidjson::kObjectType);

        instanceJson.AddMember(rapidjson::StringRef(ID), windowInstance.id, alloc);
        instanceJson.AddMember(rapidjson::StringRef(TEMPLATE_ID), windowInstance.templateId, alloc);
        instanceJson.AddMember(rapidjson::StringRef(TOKEN), m_tokenPerWindow[windowId], alloc);

        rapidjson::Value configurationJson(rapidjson::kObjectType);
        configurationJson.AddMember(rapidjson::StringRef(INTERACTION_MODE), windowInstance.interactionMode, alloc);
        configurationJson.AddMember(
            rapidjson::StringRef(SIZE_CONFIGURATION_ID), windowInstance.sizeConfigurationId, alloc);
        instanceJson.AddMember(rapidjson::StringRef(CONFIGURATION), configurationJson, alloc);

        instancesJson.PushBack(instanceJson, alloc);
    }

    payload.AddMember(rapidjson::StringRef(INSTANCES), instancesJson, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    payload.Accept(writer);

    stateJson = sb.GetString();
}

void VisualCharacteristics::initializeCapabilityConfiguration() {
    std::string interactionModeJson, displayWindowJson, displayJson;

    VCConfigParser::serializeInteractionMode(
        m_visualCharacteristicsConfiguration.interactionModes, interactionModeJson);
    VCConfigParser::serializeWindowTemplate(m_visualCharacteristicsConfiguration.windowTemplates, displayWindowJson);
    VCConfigParser::serializeDisplayCharacteristics(
        m_visualCharacteristicsConfiguration.displayCharacteristics, displayJson);

    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXAINTERACTIONMODE_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXAINTERACTIONMODE_CAPABILITY_INTERFACE_VERSION});
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, interactionModeJson});
    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));

    configMap.clear();
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXADISPLAYWINDOW_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXADISPLAYWINDOW_CAPABILITY_INTERFACE_VERSION});
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, displayWindowJson});
    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));

    configMap.clear();
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXADISPLAY_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXADISPLAY_CAPABILITY_INTERFACE_VERSION});
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, displayJson});
    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));
}
}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
