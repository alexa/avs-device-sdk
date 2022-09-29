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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/AlexaPresentationAPL/private/AlexaPresentationAPL.h"

namespace alexaClientSDK {
namespace aplCapabilityAgent {

using namespace alexaClientSDK;
using namespace alexaClientSDK::aplCapabilityCommonInterfaces;
using namespace alexaClientSDK::avsCommon::utils::timing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationAPL"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// AlexaPresentationAPL interface type
static const char* ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// AlexaPresentationAPL interface name
static const char* ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_NAME = "Alexa.Presentation.APL";

/// AlexaPresentationAPL interface version for Alexa.Presentation.APL
static const char* ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_VERSION = "1.4";

/// Alexa.Presentation.APL.Video interface name
static const char* ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_INTERFACE_NAME = "Alexa.Presentation.APL.Video";

/// Alexa.Presentation.APL.Video interface version
static const char* ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Namespace three supported by Alexa presentation APL capability agent.
static const char* ALEXA_PRESENTATION_APL_NAMESPACE = "Alexa.Presentation.APL";

/// The name for RenderDocument directive.
static const char* RENDER_DOCUMENT = "RenderDocument";

/// The name for ExecuteCommand directive.
static const char* EXECUTE_COMMAND = "ExecuteCommands";

/// The name for SendIndexListData directive.
static const char* SEND_INDEX_LIST_DATA = "SendIndexListData";

/// The name for UpdateIndexListData directive.
static const char* UPDATE_INDEX_LIST_DATA = "UpdateIndexListData";

/// The name for SendTokenListData directive.
static const char* SEND_TOKEN_LIST_DATA = "SendTokenListData";

/// The RenderDocument directive signature.
static const NamespaceAndName DOCUMENT{ALEXA_PRESENTATION_APL_NAMESPACE, RENDER_DOCUMENT};

/// The ExecuteCommand directive signature.
static const NamespaceAndName COMMAND{ALEXA_PRESENTATION_APL_NAMESPACE, EXECUTE_COMMAND};

/// The SendIndexListData directive signature.
static const NamespaceAndName INDEX_LIST_DATA{ALEXA_PRESENTATION_APL_NAMESPACE, SEND_INDEX_LIST_DATA};

/// The UpdateIndexListData directive signature.
static const NamespaceAndName INDEX_LIST_UPDATE{ALEXA_PRESENTATION_APL_NAMESPACE, UPDATE_INDEX_LIST_DATA};

/// The SendTokenListData directive signature.
static const NamespaceAndName TOKEN_LIST_DATA{ALEXA_PRESENTATION_APL_NAMESPACE, SEND_TOKEN_LIST_DATA};

/// Name of the runtime configuration.
static const char* RUNTIME_CONFIG = "runtime";

/// Identifier for the runtime (APL) version of the configuration.
static const char* APL_MAX_VERSION = "maxVersion";

/// Identifier for the skilld in presentationSession
static const char* SKILL_ID = "skillId";

/// Identifier for the id in presentationSession
static const char* PRESENTATION_SESSION_ID = "id";

/// The key in our config file to find the root of APL Presentation configuration.
static std::string ALEXAPRESENTATIONAPL_CONFIGURATION_ROOT_KEY = "AlexaPresentationAPLCapabilityAgent";

std::map<aplCapabilityCommon::BaseAPLCapabilityAgent::MetricEvent, std::string>
    AlexaPresentationAPL::METRICS_DATA_POINT_NAMES = {
        {aplCapabilityCommon::BaseAPLCapabilityAgent::MetricEvent::RENDER_DOCUMENT,
         "AlexaPresentationAPL.RenderDocument"}};

std::map<aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity, std::string>
    AlexaPresentationAPL::METRICS_ACTIVITY_NAMES = {
        {aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity::ACTIVITY_RENDER_DOCUMENT,
         "AlexaPresentationAPL.renderDocument"},
        {aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity::ACTIVITY_RENDER_DOCUMENT_FAIL,
         "AlexaPresentationAPL.renderDocument.fail"}};

std::shared_ptr<AlexaPresentationAPL> AlexaPresentationAPL::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::string APLVersion,
    std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface> visualStateProvider) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManagerInterface"));
        return nullptr;
    }

    std::shared_ptr<AlexaPresentationAPL> alexaPresentationAPL(new AlexaPresentationAPL(
        exceptionSender, metricRecorder, messageSender, contextManager, APLVersion, visualStateProvider));

    if (!alexaPresentationAPL->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization error."));
        return nullptr;
    }

    return alexaPresentationAPL;
}

AlexaPresentationAPL::AlexaPresentationAPL(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::string APLVersion,
    std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface> visualStateProvider) :
        aplCapabilityCommon::BaseAPLCapabilityAgent(
            ALEXA_PRESENTATION_APL_NAMESPACE,
            exceptionSender,
            metricRecorder,
            messageSender,
            contextManager,
            APLVersion,
            visualStateProvider) {
    m_videoSettings.codecs = {VideoSettings::Codec::H_264_41, VideoSettings::Codec::H_264_42};
}

std::shared_ptr<CapabilityConfiguration> AlexaPresentationAPL::getAlexaPresentationAPLCapabilityConfiguration(
    const std::string& APLMaxVersion) {
    if (APLMaxVersion.empty()) {
        ACSDK_ERROR(LX("getAlexaPresentationAPLCapabilityConfigurationFailed").d("reason", "empty APL Version"));
        return nullptr;
    }

    std::unordered_map<std::string, std::string> configMap;

    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_VERSION});

    rapidjson::Document runtime(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc = runtime.GetAllocator();

    rapidjson::Value configJson;
    configJson.SetObject();

    configJson.AddMember(rapidjson::StringRef(APL_MAX_VERSION), APLMaxVersion, alloc);
    runtime.AddMember(rapidjson::StringRef(RUNTIME_CONFIG), configJson, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!runtime.Accept(writer)) {
        ACSDK_CRITICAL(
            LX("getAlexaPresentationAPLCapabilityConfigurationFailed").d("reason", "configWriterRefusedJsonObject"));
        return nullptr;
    }

    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, buffer.GetString()});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::shared_ptr<CapabilityConfiguration> AlexaPresentationAPL::getAlexaPresentationVideoCapabilityConfiguration() {
    std::string serializedVideoConfiguration;
    if (!AlexaPresentationAPLVideoConfigParser::serializeVideoSettings(m_videoSettings, serializedVideoConfiguration)) {
        ACSDK_CRITICAL(LX("getAlexaPresentationVideoCapabilityConfigurationFailed")
                           .d("reason", "video config serialization failed"));
        return nullptr;
    }
    ACSDK_DEBUG5(LX(__func__).d("videoSettingsConfiguration", serializedVideoConfiguration));

    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, ALEXAPRESENTATIONAPL_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_INTERFACE_VERSION});
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, serializedVideoConfiguration});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

DirectiveHandlerConfiguration AlexaPresentationAPL::getAPLDirectiveConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;

    configuration[DOCUMENT] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, true);
    configuration[COMMAND] = BlockingPolicy(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, true);
    configuration[INDEX_LIST_DATA] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, false);
    configuration[INDEX_LIST_UPDATE] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, false);
    configuration[TOKEN_LIST_DATA] = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, false);
    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaPresentationAPL::
    getAPLCapabilityConfigurations(const std::string& APLMaxVersion) {
    m_capabilityConfigurations.insert(getAlexaPresentationAPLCapabilityConfiguration(APLMaxVersion));
    m_capabilityConfigurations.insert(getAlexaPresentationVideoCapabilityConfiguration());
    return m_capabilityConfigurations;
}

aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType AlexaPresentationAPL::getDirectiveType(
    std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::UNKNOWN;
    }

    if (info->directive->getNamespace() == DOCUMENT.nameSpace && info->directive->getName() == DOCUMENT.name) {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::RENDER_DOCUMENT;
    } else if (info->directive->getNamespace() == COMMAND.nameSpace && info->directive->getName() == COMMAND.name) {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::EXECUTE_COMMAND;
    } else if (
        (info->directive->getNamespace() == INDEX_LIST_DATA.nameSpace &&
         info->directive->getName() == INDEX_LIST_DATA.name) ||
        (info->directive->getNamespace() == INDEX_LIST_UPDATE.nameSpace &&
         info->directive->getName() == INDEX_LIST_UPDATE.name)) {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::DYNAMIC_INDEX_DATA_SOURCE_UPDATE;
    } else if (
        info->directive->getNamespace() == TOKEN_LIST_DATA.nameSpace &&
        info->directive->getName() == TOKEN_LIST_DATA.name) {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::DYNAMIC_TOKEN_DATA_SOURCE_UPDATE;
    } else {
        return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::UNKNOWN;
    }
}

const std::string& AlexaPresentationAPL::getConfigurationRootKey() {
    return ALEXAPRESENTATIONAPL_CONFIGURATION_ROOT_KEY;
}

const std::string& AlexaPresentationAPL::getMetricDataPointName(
    aplCapabilityCommon::BaseAPLCapabilityAgent::MetricEvent event) {
    return METRICS_DATA_POINT_NAMES[event];
}

const std::string& AlexaPresentationAPL::getMetricActivityName(
    aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity activity) {
    return METRICS_ACTIVITY_NAMES[activity];
}

aplCapabilityCommon::BaseAPLCapabilityAgent::PresentationSessionFieldNames AlexaPresentationAPL::
    getPresentationSessionFieldNames() {
    aplCapabilityCommon::BaseAPLCapabilityAgent::PresentationSessionFieldNames fieldNames;
    fieldNames.skillId = SKILL_ID;
    fieldNames.presentationSessionId = PRESENTATION_SESSION_ID;
    return fieldNames;
}

const bool AlexaPresentationAPL::shouldPackPresentationSessionToAvsEvents() {
    return false;
}

bool AlexaPresentationAPL::initialize() {
    if (!aplCapabilityCommon::BaseAPLCapabilityAgent::initialize()) {
        ACSDK_ERROR(LX(__func__).m("BaseAPLCapabilityAgent initialization failed"));
        return false;
    }

    auto configurationRoot = ConfigurationNode::getRoot()[getConfigurationRootKey()];
    VideoSettings videoSettings;
    if (AlexaPresentationAPLVideoConfigParser::parseVideoSettings(configurationRoot, videoSettings)) {
        ACSDK_DEBUG5(LX(__func__).m("Using video settings from config"));
        m_videoSettings = videoSettings;
    }

    return true;
}

}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK
