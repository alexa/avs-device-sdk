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

#include "MRM/MRMCapabilityAgent.h"

#include <AVSCommon/Utils/Logger/Logger.h>

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

/// The namespace for this capability agent.
static const std::string NAMESPACE_STR = "MRM";

/// The wildcard namespace signature so the DirectiveSequencer will send us all Directives under the namespace.
static const avsCommon::avs::NamespaceAndName WHA_NAMESPACE_WILDCARD{NAMESPACE_STR, "*"};

/**
 * Creates the MRM capability configuration, needed to register with Device Capability Framework.
 *
 * @return The MRM capability configuration.
 */
static std::shared_ptr<CapabilityConfiguration> getMRMCapabilityConfiguration() {
    static const std::unordered_map<std::string, std::string> configMap;
    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::shared_ptr<MRMCapabilityAgent> MRMCapabilityAgent::create(
    std::unique_ptr<MRMHandlerInterface> mrmHandler,
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

    auto agent = std::shared_ptr<MRMCapabilityAgent>(new MRMCapabilityAgent(
        std::move(mrmHandler), speakerManager, userInactivityMonitor, exceptionEncounteredSender));

    userInactivityMonitor->addObserver(agent);
    speakerManager->addSpeakerManagerObserver(agent);

    return agent;
}

MRMCapabilityAgent::MRMCapabilityAgent(
    std::unique_ptr<MRMHandlerInterface> handler,
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<UserInactivityMonitorInterface> userInactivityMonitor,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent(NAMESPACE_STR, exceptionEncounteredSender),
        RequiresShutdown("MRMCapabilityAgent"),
        m_mrmHandler{std::move(handler)},
        m_speakerManager{speakerManager},
        m_userInactivityMonitor{userInactivityMonitor} {
    ACSDK_DEBUG5(LX(__func__));
};

MRMCapabilityAgent::~MRMCapabilityAgent() {
    ACSDK_DEBUG5(LX(__func__));
}

void MRMCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    // intentional no-op.
}

void MRMCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "info is nullptr."));
        return;
    }
    m_executor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

void MRMCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    // intentional no-op.
}

void MRMCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
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
    // TODO: ARC-227 verify default values
    configuration[WHA_NAMESPACE_WILDCARD] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

void MRMCapabilityAgent::onUserInactivityReportSent() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this]() { executeOnUserInactivityReportSent(); });
}

void MRMCapabilityAgent::onSpeakerSettingsChanged(
    const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
    const avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
    const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) {
    ACSDK_DEBUG5(LX(__func__).d("type", type));
    m_executor.submit([this, type]() { executeOnSpeakerSettingsChanged(type); });
}

std::string MRMCapabilityAgent::getVersionString() const {
    ACSDK_DEBUG5(LX(__func__));
    return m_mrmHandler->getVersionString();
}

void MRMCapabilityAgent::executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));

    if (m_mrmHandler->handleDirective(info->directive)) {
        if (info->result) {
            info->result->setCompleted();
        }
    } else {
        std::string errorMessage = "MRM Handler was unable to handle Directive - " + info->directive->getNamespace() +
                                   ":" + info->directive->getName();
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(), avsCommon::avs::ExceptionErrorType::INTERNAL_ERROR, errorMessage);
        ACSDK_ERROR(LX("executeHandleDirectiveImmediatelyFailed").d("reason", errorMessage));

        if (info->result) {
            info->result->setFailed(errorMessage);
        }
    }

    removeDirective(info->directive->getMessageId());
}

void MRMCapabilityAgent::executeOnSpeakerSettingsChanged(const avsCommon::sdkInterfaces::SpeakerInterface::Type& type) {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->onSpeakerSettingsChanged(type);
}

void MRMCapabilityAgent::executeOnUserInactivityReportSent() {
    ACSDK_DEBUG5(LX(__func__));
    m_mrmHandler->onUserInactivityReportSent();
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> MRMCapabilityAgent::getCapabilityConfigurations() {
    std::unordered_set<std::shared_ptr<CapabilityConfiguration>> configs;
    configs.insert(getMRMCapabilityConfiguration());
    return configs;
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