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
#include <utility>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "AFML/AudioActivityTracker.h"

namespace alexaClientSDK {
namespace afml {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("AudioActivityTracker");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The state information @c NamespaceAndName to send to the @c ContextManager for AudioActivityTracker.
static const NamespaceAndName CONTEXT_MANAGER_STATE{"AudioActivityTracker", "ActivityState"};

/// The idleTime key used in the AudioActivityTracker context.
static const char IDLE_TIME_KEY[] = "idleTimeInMilliseconds";

/// The interface key used in the AudioActivityTracker context.
static const char INTERFACE_KEY[] = "interface";

std::shared_ptr<AudioActivityTracker> AudioActivityTracker::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }

    auto audioActivityTracker = std::shared_ptr<AudioActivityTracker>(new AudioActivityTracker(contextManager));

    contextManager->setStateProvider(CONTEXT_MANAGER_STATE, audioActivityTracker);

    return audioActivityTracker;
}

void AudioActivityTracker::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    ACSDK_DEBUG5(LX("provideState"));
    m_executor.submit([this, stateRequestToken]() { executeProvideState(stateRequestToken); });
}

void AudioActivityTracker::notifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) {
    ACSDK_DEBUG5(LX("notifyOfActivityUpdates"));
    m_executor.submit([this, channelStates]() { executeNotifyOfActivityUpdates(channelStates); });
}

AudioActivityTracker::AudioActivityTracker(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) :
        RequiresShutdown{"AudioActivityTracker"},
        m_contextManager{contextManager} {
}

void AudioActivityTracker::doShutdown() {
    m_executor.shutdown();
    m_contextManager.reset();
}

void AudioActivityTracker::executeNotifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) {
    ACSDK_DEBUG5(LX("executeNotifyOfActivityUpdates"));
    for (const auto& state : channelStates) {
        /*
         * Special logic to ignore the SpeechRecognizer interface as specified by the Focus Management requirements.
         */
        if ("SpeechRecognizer" == state.interfaceName && FocusManagerInterface::DIALOG_CHANNEL_NAME == state.name) {
            continue;
        }
        m_channelStates[state.name] = state;
    }
}

void AudioActivityTracker::executeProvideState(unsigned int stateRequestToken) {
    ACSDK_DEBUG5(LX("executeProvideState"));
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    auto currentTime = std::chrono::steady_clock::now();
    auto sendEmptyContext = true;

    if (!m_channelStates.empty()) {
        for (auto it = m_channelStates.begin(); it != m_channelStates.end(); ++it) {
            rapidjson::Value contextJson(rapidjson::kObjectType);
            auto idleTime = std::chrono::milliseconds::zero();
            auto& channelContext = it->second;

            if (FocusState::NONE == channelContext.focusState) {
                idleTime =
                    std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - channelContext.timeAtIdle);
            }

            contextJson.AddMember(INTERFACE_KEY, channelContext.interfaceName, payload.GetAllocator());
            contextJson.AddMember(IDLE_TIME_KEY, idleTime.count(), payload.GetAllocator());

            payload.AddMember(
                rapidjson::StringRef(executeChannelNameInLowerCase(it->first)), contextJson, payload.GetAllocator());
        }

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

const std::string& AudioActivityTracker::executeChannelNameInLowerCase(const std::string& channelName) {
    auto it = m_channelNamesInLowerCase.find(channelName);

    // Store channel name in lower case if not done so yet.
    if (it == m_channelNamesInLowerCase.end()) {
        auto channelNameInLowerCase = avsCommon::utils::string::stringToLowerCase(channelName);
        it = m_channelNamesInLowerCase.insert(it, std::make_pair(channelName, channelNameInLowerCase));
    }
    return it->second;
}

}  // namespace afml
}  // namespace alexaClientSDK
