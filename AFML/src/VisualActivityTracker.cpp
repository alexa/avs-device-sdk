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
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "AFML/VisualActivityTracker.h"

namespace alexaClientSDK {
namespace afml {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("VisualActivityTracker");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The state information @c NamespaceAndName to send to the @c ContextManager for VisualActivityTracker.
static const NamespaceAndName CONTEXT_MANAGER_STATE{"VisualActivityTracker", "ActivityState"};

/// The interface key used in the VisualActivityTracker context.
static const char FOCUSED_KEY[] = "focused";

/// The interface key used in the VisualActivityTracker context.
static const char INTERFACE_KEY[] = "interface";

std::shared_ptr<VisualActivityTracker> VisualActivityTracker::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }

    auto visualActivityTracker = std::shared_ptr<VisualActivityTracker>(new VisualActivityTracker(contextManager));

    contextManager->setStateProvider(CONTEXT_MANAGER_STATE, visualActivityTracker);

    return visualActivityTracker;
}

void VisualActivityTracker::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    ACSDK_DEBUG5(LX("provideState"));
    m_executor.submit([this, stateRequestToken]() { executeProvideState(stateRequestToken); });
}

void VisualActivityTracker::notifyOfActivityUpdates(const std::vector<Channel::State>& channels) {
    ACSDK_DEBUG5(LX("notifyOfActivityUpdates"));
    // Return error if vector is empty.
    if (channels.empty()) {
        ACSDK_WARN(LX("notifyOfActivityUpdates").d("reason", "emptyVector"));
        return;
    }

    /*
     * Currently we only have one visual channel, so we log an error if we are not getting updates with the expected
     * one.
     */
    for (auto& channel : channels) {
        if (FocusManagerInterface::VISUAL_CHANNEL_NAME != channel.name) {
            ACSDK_ERROR(LX("notifyOfActivityUpdates").d("reason", "InvalidChannelName").d("name", channel.name));
            return;
        }
    }

    m_executor.submit([this, channels]() {
        // The last element of the vector is the most recent channel state.
        m_channelState = channels.back();
    });
}

VisualActivityTracker::VisualActivityTracker(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) :
        RequiresShutdown{"VisualActivityTracker"},
        m_contextManager{contextManager} {
}

void VisualActivityTracker::doShutdown() {
    m_executor.shutdown();
    m_contextManager.reset();
}

void VisualActivityTracker::executeProvideState(unsigned int stateRequestToken) {
    ACSDK_DEBUG5(LX("executeProvideState"));
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    auto sendEmptyContext = true;

    if (FocusState::NONE != m_channelState.focusState) {
        rapidjson::Value contextJson(rapidjson::kObjectType);
        contextJson.AddMember(INTERFACE_KEY, m_channelState.interfaceName, payload.GetAllocator());
        payload.AddMember(FOCUSED_KEY, contextJson, payload.GetAllocator());

        if (!payload.Accept(writer)) {
            ACSDK_ERROR(LX("executeProvideState").d("reason", "writerRefusedJsonObject"));
        } else {
            sendEmptyContext = false;
        }
    }

    if (sendEmptyContext) {
        m_contextManager->setState(
            CONTEXT_MANAGER_STATE, "", avsCommon::avs::StateRefreshPolicy::SOMETIMES, stateRequestToken);
    } else {
        m_contextManager->setState(
            CONTEXT_MANAGER_STATE,
            buffer.GetString(),
            avsCommon::avs::StateRefreshPolicy::SOMETIMES,
            stateRequestToken);
    }
}

}  // namespace afml
}  // namespace alexaClientSDK
