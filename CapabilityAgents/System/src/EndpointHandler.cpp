/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "System/EndpointHandler.h"

#include <string>
#include <rapidjson/document.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"EndpointHandler"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for AVS enpointing.
static const std::string ENDPOINTING_NAMESPACE = "System";

/// This string holds the name of the directive that's being sent for endpointing.
static const std::string ENDPOINTING_NAME = "SetEndpoint";

/// This string holds the key for the endpoint in the payload.
static const std::string ENDPOINT_PAYLOAD_KEY = "endpoint";

void EndpointHandler::removeDirectiveGracefully(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    bool isFailure,
    const std::string& report) {
    if (info) {
        if (info->result) {
            if (isFailure) {
                info->result->setFailed(report);
            } else {
                info->result->setCompleted();
            }
            if (info->directive) {
                CapabilityAgent::removeDirective(info->directive->getMessageId());
            }
        }
    }
}

std::shared_ptr<EndpointHandler> EndpointHandler::create(
    std::shared_ptr<AVSEndpointAssignerInterface> avsEndpointAssigner,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    if (!avsEndpointAssigner) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAvsEndpointInterface"));
        return nullptr;
    }
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }
    return std::shared_ptr<EndpointHandler>(new EndpointHandler(avsEndpointAssigner, exceptionEncounteredSender));
}

EndpointHandler::EndpointHandler(
    std::shared_ptr<AVSEndpointAssignerInterface> avsEndpointAssigner,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent(ENDPOINTING_NAMESPACE, exceptionEncounteredSender),
        m_avsEndpointAssigner{avsEndpointAssigner} {
}

avsCommon::avs::DirectiveHandlerConfiguration EndpointHandler::getConfiguration() const {
    return avsCommon::avs::DirectiveHandlerConfiguration{{NamespaceAndName{ENDPOINTING_NAMESPACE, ENDPOINTING_NAME},
                                                          BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

void EndpointHandler::preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
}

void EndpointHandler::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<avsCommon::avs::CapabilityAgent::DirectiveInfo>(directive, nullptr));
}

void EndpointHandler::handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInDirectiveInfo"));
        return;
    }
    std::string newEndpoint;
    if (!jsonUtils::retrieveValue(info->directive->getPayload(), ENDPOINT_PAYLOAD_KEY, &newEndpoint)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "payloadMissingEndpointKey"));
        removeDirectiveGracefully(info, true, "payloadMissingEndpointKey");
    } else {
        m_avsEndpointAssigner->setAVSEndpoint(newEndpoint);
        removeDirectiveGracefully(info);
    }
}

void EndpointHandler::cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    if (!info->directive) {
        removeDirectiveGracefully(info, true, "nullDirective");
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInDirectiveInfo"));
        return;
    }
    CapabilityAgent::removeDirective(info->directive->getMessageId());
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
