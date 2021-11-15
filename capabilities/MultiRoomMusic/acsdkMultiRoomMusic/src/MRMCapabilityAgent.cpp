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

#include <chrono>
#include <rapidjson/document.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkMultiRoomMusic/MRMCapabilityAgent.h"

/// String to identify log entries originating from this file.
static const std::string TAG = "MRMCapabilityAgent";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace capabilityAgents {
namespace mrm {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;

/// The namespace for this capability agent.
static const std::string CAPABILITY_AGENT_NAMESPACE_STR = "MRM";

/// Directive namespaces that this capability agent accepts
static const std::string DIRECTIVE_NAMESPACE_STR = "WholeHomeAudio";
/// Directives under this namespace are for controlling output device skews(bluetooth)
static const std::string SKEW_DIRECTIVE_NAMESPACE_STR = "WholeHomeAudio.Skew";

/// The wildcard namespace signature so the DirectiveSequencer will send us all
/// Directives under the namespace.
static const avsCommon::avs::NamespaceAndName WHA_NAMESPACE_WILDCARD{DIRECTIVE_NAMESPACE_STR, "*"};
static const avsCommon::avs::NamespaceAndName WHA_SKEW_NAMESPACE_WILDCARD{SKEW_DIRECTIVE_NAMESPACE_STR, "*"};

/// The key in our config file to find the root of MRM for this database.
static const std::string MRM_CONFIGURATION_ROOT_KEY = "mrm";
/// The key in our config file to find the MRM capabilities.
static const std::string MRM_CAPABILITIES_KEY = "capabilities";

/// The amount of time to delay the processing of alexa dialog state changes in an effort to improve
/// WakeWordToBar performance, by freeing up resources during the critical time just after a wake word.
static const std::chrono::milliseconds DIALOG_STATE_UPDATE_DELAY{200};

static std::unordered_set<std::shared_ptr<CapabilityConfiguration>> readCapabilities() {
    std::unordered_set<std::shared_ptr<CapabilityConfiguration>> capabilitiesSet;
    auto configRoot = configuration::ConfigurationNode::getRoot();
    if (!configRoot) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "configurationRootNotFound"));
        return capabilitiesSet;
    }

    auto mrmConfig = configRoot[MRM_CONFIGURATION_ROOT_KEY];
    if (!mrmConfig) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "configurationKeyNotFound")
                        .d("configurationKey", MRM_CONFIGURATION_ROOT_KEY));
        return capabilitiesSet;
    }

    auto capabilitiesConfig = mrmConfig[MRM_CAPABILITIES_KEY];
    if (!capabilitiesConfig) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "capabilitiesKeyNotFound").d("key", MRM_CAPABILITIES_KEY));
        return capabilitiesSet;
    }

    std::string capabilitiesString = capabilitiesConfig.serialize();
    rapidjson::Document capabilities;
    if (!jsonUtils::parseJSON(capabilitiesString, &capabilities)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "failedToParseCapabilitiesString")
                        .d("capabilitiesString", capabilitiesString));
        return capabilitiesSet;
    }
    for (auto itr = capabilities.MemberBegin(); itr != capabilities.MemberEnd(); ++itr) {
        std::string interfaceType;
        if (!jsonUtils::retrieveValue(itr->value, CAPABILITY_INTERFACE_TYPE_KEY, &interfaceType)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "failedToFindCapabilityInterfaceTypeKey")
                            .d("key", CAPABILITY_INTERFACE_TYPE_KEY));
            return capabilitiesSet;
        }

        std::string interfaceName;
        if (!jsonUtils::retrieveValue(itr->value, CAPABILITY_INTERFACE_NAME_KEY, &interfaceName)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "failedToFindCapabilityInterfaceNameKey")
                            .d("key", CAPABILITY_INTERFACE_NAME_KEY));
            return capabilitiesSet;
        }

        std::string interfaceVersion;
        if (!jsonUtils::retrieveValue(itr->value, CAPABILITY_INTERFACE_VERSION_KEY, &interfaceVersion)) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "failedToFindCapabilityInterfaceVersionKey")
                            .d("key", CAPABILITY_INTERFACE_VERSION_KEY));
            return capabilitiesSet;
        }

        // Configurations is an optional field.
        std::string configurationsString;
        rapidjson::Value::ConstMemberIterator configurations;
        if (jsonUtils::findNode(itr->value, CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, &configurations)) {
            if (!jsonUtils::convertToValue(configurations->value, &configurationsString)) {
                ACSDK_ERROR(LX("initializeFailed").d("reason", "failedToConvertConfigurations"));
                return capabilitiesSet;
            }
        }

        std::unordered_map<std::string, std::string> capabilityMap = {
            {CAPABILITY_INTERFACE_TYPE_KEY, interfaceType},
            {CAPABILITY_INTERFACE_NAME_KEY, interfaceName},
            {CAPABILITY_INTERFACE_VERSION_KEY, interfaceVersion}};
        if (!configurationsString.empty()) {
            capabilityMap[CAPABILITY_INTERFACE_CONFIGURATIONS_KEY] = configurationsString;
        }
        capabilitiesSet.insert(std::make_shared<CapabilityConfiguration>(capabilityMap));
    }

    if (capabilitiesSet.empty()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "missingCapabilityConfigurations"));
    }

    return capabilitiesSet;
}

std::shared_ptr<MRMCapabilityAgent> MRMCapabilityAgent::create(
    std::shared_ptr<MRMHandlerInterface> mrmHandler,
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    ACSDK_DEBUG5(LX(__func__));

    if (!mrmHandler) {
        ACSDK_ERROR(LX("createFailed").d("reason", "mrmHandler was nullptr."));
        return nullptr;
    }
    if (!speakerManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "speakerManager was nullptr."));
        return nullptr;
    }
    if (!userInactivityMonitor) {
        ACSDK_ERROR(LX("createFailed").d("reason", "userInactivityMonitor was nullptr."));
        return nullptr;
    }
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionEncounteredSender was nullptr."));
        return nullptr;
    }

    auto agent = std::shared_ptr<MRMCapabilityAgent>(
        new MRMCapabilityAgent(mrmHandler, speakerManager, userInactivityMonitor, exceptionEncounteredSender));

    userInactivityMonitor->addObserver(agent);
    speakerManager->addSpeakerManagerObserver(agent);

    return agent;
}

MRMCapabilityAgent::MRMCapabilityAgent(
    std::shared_ptr<MRMHandlerInterface> handler,
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent(CAPABILITY_AGENT_NAMESPACE_STR, exceptionEncounteredSender),
        RequiresShutdown("MRMCapabilityAgent"),
        m_mrmHandler{handler},
        m_speakerManager{speakerManager},
        m_userInactivityMonitor{userInactivityMonitor},
        m_wasPreviouslyActive{false} {
    ACSDK_DEBUG5(LX(__func__));
};

MRMCapabilityAgent::~MRMCapabilityAgent() {
    ACSDK_DEBUG5(LX(__func__));
}

void MRMCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // intentional no-op.
}
void MRMCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("handleDirective"));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "info is nullptr."));
        return;
    }
    m_executor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

void MRMCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("cancelDirective"));
    // intentional no-op.
}

void MRMCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG5(LX("handleDirectiveImmediately"));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "directive is nullptr."));
        return;
    }
    auto info = std::make_shared<DirectiveInfo>(directive, nullptr);
    m_executor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

DirectiveHandlerConfiguration MRMCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    configuration[WHA_NAMESPACE_WILDCARD] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    configuration[WHA_SKEW_NAMESPACE_WILDCARD] = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    return configuration;
}

void MRMCapabilityAgent::onUserInactivityReportSent() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this]() { executeOnUserInactivityReportSent(); });
}

void MRMCapabilityAgent::onSpeakerSettingsChanged(
    const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
    const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
    const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__).d("type", type));
    m_executor.submit([this, source, type, settings]() { executeOnSpeakerSettingsChanged(source, type, settings); });
}

void MRMCapabilityAgent::onCallStateChange(avsCommon::sdkInterfaces::CallStateObserverInterface::CallState callState) {
    ACSDK_DEBUG5(LX(__func__).d("callState", callState));
    m_executor.submit([this, callState]() { executeOnCallStateChange(callState); });
}

void MRMCapabilityAgent::onDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState state) {
    ACSDK_DEBUG5(LX(__func__).d("state", state));
    m_delayedTaskTimer.submitTask(DIALOG_STATE_UPDATE_DELAY, [this, state]() { executeOnDialogUXStateChanged(state); });
}

std::string MRMCapabilityAgent::getVersionString() const {
    ACSDK_DEBUG5(LX(__func__));
    return m_mrmHandler->getVersionString();
}

void MRMCapabilityAgent::setObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("setObserverFailed").m("Observer is null."));
        return;
    }

    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, observer]() { executeSetObserver(observer); });
}

void MRMCapabilityAgent::executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));

    if (m_mrmHandler->handleDirective(
            info->directive->getNamespace(),
            info->directive->getName(),
            info->directive->getMessageId(),
            info->directive->getPayload())) {
        if (info->result) {
            info->result->setCompleted();
        }
    } else {
        std::string errorMessage = "MultiRoomMusic Handler was unable to handle Directive - " +
                                   info->directive->getNamespace() + ":" + info->directive->getName();
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(), avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, errorMessage);
        ACSDK_ERROR(LX("executeHandleDirectiveImmediatelyFailed").d("reason", errorMessage));

        if (info->result) {
            info->result->setFailed(errorMessage);
        }
    }

    removeDirective(info->directive->getMessageId());
}

void MRMCapabilityAgent::executeOnSpeakerSettingsChanged(
    const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
    const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
    const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->onSpeakerSettingsChanged(source, type, settings);
}

void MRMCapabilityAgent::executeOnUserInactivityReportSent() {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->onUserInactivityReportSent();
}

void MRMCapabilityAgent::executeOnCallStateChange(
    const avsCommon::sdkInterfaces::CallStateObserverInterface::CallState callState) {
    ACSDK_DEBUG5(LX(__func__));
    bool isCurrentlyActive = CallStateObserverInterface::isStateActive(callState);

    // Only send down the CallStateChange event if the active call state changed
    if (m_wasPreviouslyActive != isCurrentlyActive) {
        m_mrmHandler->onCallStateChange(isCurrentlyActive);
        m_wasPreviouslyActive = isCurrentlyActive;
    } else {
        ACSDK_WARN(LX(__func__).m("call active state didn't actually change"));
    }
}

void MRMCapabilityAgent::executeOnDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState state) {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->onDialogUXStateChanged(state);
}

void MRMCapabilityAgent::executeSetObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->setObserver(observer);
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> MRMCapabilityAgent::getCapabilityConfigurations() {
    return readCapabilities();
}

void MRMCapabilityAgent::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));
    m_speakerManager->removeSpeakerManagerObserver(shared_from_this());
    m_speakerManager.reset();
    m_userInactivityMonitor->removeObserver(shared_from_this());
    m_userInactivityMonitor.reset();
    m_mrmHandler->shutdown();
}

}  // namespace mrm
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
