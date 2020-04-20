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

#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "TemplateRuntime/TemplateRuntime.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace templateRuntime {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;

/// TemplateRuntime capability constants
/// TemplateRuntime interface type
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// TemplateRuntime interface name
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_NAME = "TemplateRuntime";
/// TemplateRuntime interface version
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_VERSION = "1.1";

/// String to identify log entries originating from this file.
static const std::string TAG{"TemplateRuntime"};

/// The key in our config file to find the root of template runtime configuration.
static const std::string TEMPLATERUNTIME_CONFIGURATION_ROOT_KEY = "templateRuntimeCapabilityAgent";
/// The key in our config file to set the display card timeout value when TTS is in FINISHED state
static const std::string TEMPLATERUNTIME_TTS_FINISHED_KEY = "displayCardTTSFinishedTimeout";
/// The key in our config file to set the display card timeout value when AudioPlayer is in FINISHED state
static const std::string TEMPLATERUNTIME_AUDIOPLAYBACK_FINISHED_KEY = "displayCardAudioPlaybackFinishedTimeout";
/// The key in our config file to set the display card timeout value when AudioPlayer is in STOPPED or PAUSE state
static const std::string TEMPLATERUNTIME_AUDIOPLAYBACK_STOPPED_PAUSED_KEY =
    "displayCardAudioPlaybackStoppedPausedTimeout";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the @c FocusManager channel used by @c TemplateRuntime.
static const std::string CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::VISUAL_CHANNEL_NAME;

/// The namespace for this capability agent.
static const std::string NAMESPACE{"TemplateRuntime"};

/// The name for RenderTemplate directive.
static const std::string RENDER_TEMPLATE{"RenderTemplate"};

/// The name for RenderPlayerInfo directive.
static const std::string RENDER_PLAYER_INFO{"RenderPlayerInfo"};

/// The RenderTemplate directive signature.
static const NamespaceAndName TEMPLATE{NAMESPACE, RENDER_TEMPLATE};

/// The RenderPlayerInfo directive signature.
static const NamespaceAndName PLAYER_INFO{NAMESPACE, RENDER_PLAYER_INFO};

/// Tag for find the AudioItemId in the payload of the RenderPlayerInfo directive
static const std::string AUDIO_ITEM_ID_TAG{"audioItemId"};

/// Maximum queue size allowed for m_audioItems.
static const size_t MAXIMUM_QUEUE_SIZE{100};

/// Default timeout for clearing the RenderTemplate display card when SpeechSynthesizer is in FINISHED state.
static const std::chrono::milliseconds DEFAULT_TTS_FINISHED_TIMEOUT_MS{2000};

/// Default timeout for clearing the RenderPlayerInfo display card when AudioPlayer is in FINISHED state.
static const std::chrono::milliseconds DEFAULT_AUDIO_FINISHED_TIMEOUT_MS{2000};

/// Default timeout for clearing the RenderPlayerInfo display card when AudioPlayer is in STOPPED/PAUSED state.
static const std::chrono::milliseconds DEFAULT_AUDIO_STOPPED_PAUSED_TIMEOUT_MS{60000};

/**
 * Creates the TemplateRuntime capability configuration.
 *
 * @return The TemplateRuntime capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getTemplateRuntimeCapabilityConfiguration();

std::shared_ptr<TemplateRuntime> TemplateRuntime::create(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>&
        renderPlayerInfoCardInterface,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender) {
    if (!focusManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullFocusManager"));
        return nullptr;
    }

    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }
    std::shared_ptr<TemplateRuntime> templateRuntime(
        new TemplateRuntime(renderPlayerInfoCardInterface, focusManager, exceptionSender));

    if (!templateRuntime->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization error."));
        return nullptr;
    }

    for (const auto& renderPlayerInfoCardProvider : renderPlayerInfoCardInterface) {
        if (!renderPlayerInfoCardProvider) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullRenderPlayerInfoCardInterface"));
            return nullptr;
        }
        renderPlayerInfoCardProvider->setObserver(templateRuntime);
    }

    return templateRuntime;
}

/**
 * Initializes the object by reading the values from configuration.
 */
bool TemplateRuntime::initialize() {
    auto configurationRoot = ConfigurationNode::getRoot()[TEMPLATERUNTIME_CONFIGURATION_ROOT_KEY];

    // If key is present, then read and initilize the value from config or set to default.
    configurationRoot.getDuration<std::chrono::milliseconds>(
        TEMPLATERUNTIME_TTS_FINISHED_KEY, &m_ttsFinishedTimeout, DEFAULT_TTS_FINISHED_TIMEOUT_MS);

    // If key is present, then read and initilize the value from config or set to default.
    configurationRoot.getDuration<std::chrono::milliseconds>(
        TEMPLATERUNTIME_AUDIOPLAYBACK_FINISHED_KEY, &m_audioPlaybackFinishedTimeout, DEFAULT_AUDIO_FINISHED_TIMEOUT_MS);

    // If key is present, then read and initilize the value from config or set to default.
    configurationRoot.getDuration<std::chrono::milliseconds>(
        TEMPLATERUNTIME_AUDIOPLAYBACK_STOPPED_PAUSED_KEY,
        &m_audioPlaybackStoppedPausedTimeout,
        DEFAULT_AUDIO_STOPPED_PAUSED_TIMEOUT_MS);

    return true;
}

void TemplateRuntime::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX("handleDirectiveImmediately"));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void TemplateRuntime::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void TemplateRuntime::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("handleDirective"));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (info->directive->getName() == TEMPLATE.name) {
        handleRenderTemplateDirective(info);
    } else if (info->directive->getName() == PLAYER_INFO.name) {
        handleRenderPlayerInfoDirective(info);
    } else {
        handleUnknownDirective(info);
    }
}

void TemplateRuntime::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

DirectiveHandlerConfiguration TemplateRuntime::getConfiguration() const {
    ACSDK_DEBUG5(LX("getConfiguration"));
    DirectiveHandlerConfiguration configuration;
    auto visualNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_VISUAL, false);

    configuration[TEMPLATE] = visualNonBlockingPolicy;
    configuration[PLAYER_INFO] = visualNonBlockingPolicy;
    return configuration;
}

void TemplateRuntime::onFocusChanged(avsCommon::avs::FocusState newFocus, MixingBehavior) {
    m_executor.submit([this, newFocus]() { executeOnFocusChangedEvent(newFocus); });
}

void TemplateRuntime::onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity state, const Context& context) {
    ACSDK_DEBUG5(LX("onRenderPlayerCardsInfoChanged"));
    m_executor.submit([this, state, context]() {
        ACSDK_DEBUG5(LX("onPlayerActivityChangedInExecutor"));
        executeAudioPlayerInfoUpdates(state, context);
    });
}

void TemplateRuntime::onDialogUXStateChanged(
    avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) {
    ACSDK_DEBUG5(LX("onDialogUXStateChanged").d("state", newState));
    m_executor.submit([this, newState]() {
        if (TemplateRuntime::State::DISPLAYING == m_state && m_lastDisplayedDirective &&
            m_lastDisplayedDirective->directive->getName() == RENDER_TEMPLATE) {
            if (avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE == newState) {
                executeStartTimer(m_ttsFinishedTimeout);
            } else if (
                avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::EXPECTING == newState ||
                avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::SPEAKING == newState) {
                executeStopTimer();
            }
        }
    });
}

void TemplateRuntime::addObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("addObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer]() {
        ACSDK_DEBUG5(LX("addObserverInExecutor"));
        if (!m_observers.insert(observer).second) {
            ACSDK_ERROR(LX("addObserverInExecutor").m("Duplicate observer."));
        }
    });
}

void TemplateRuntime::removeObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("removeObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer]() {
        ACSDK_DEBUG5(LX("removeObserverInExecutor"));
        if (m_observers.erase(observer) == 0) {
            ACSDK_WARN(LX("removeObserverInExecutor").m("Nonexistent observer."));
        }
    });
}

TemplateRuntime::TemplateRuntime(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>&
        renderPlayerInfoCardsInterfaces,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"TemplateRuntime"},
        m_isRenderTemplateLastReceived{false},
        m_focus{FocusState::NONE},
        m_state{TemplateRuntime::State::IDLE},
        m_renderPlayerInfoCardsInterfaces{renderPlayerInfoCardsInterfaces},
        m_focusManager{focusManager} {
    m_capabilityConfigurations.insert(getTemplateRuntimeCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getTemplateRuntimeCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, TEMPLATERUNTIME_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, TEMPLATERUNTIME_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, TEMPLATERUNTIME_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

void TemplateRuntime::doShutdown() {
    m_executor.shutdown();
    m_focusManager.reset();
    m_observers.clear();
    m_activeRenderPlayerInfoCardsProvider.reset();
    m_audioItemsInExecution.clear();
    m_audioPlayerInfo.clear();
    for (const auto renderPlayerInfoCardsInterface : m_renderPlayerInfoCardsInterfaces) {
        renderPlayerInfoCardsInterface->setObserver(nullptr);
    }
    m_renderPlayerInfoCardsInterfaces.clear();
}

void TemplateRuntime::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    /*
     * Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
     * In those cases there is no messageId to remove because no result was expected.
     */
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void TemplateRuntime::displayCardCleared() {
    m_executor.submit([this]() { executeCardClearedEvent(); });
}

void TemplateRuntime::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void TemplateRuntime::handleRenderTemplateDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("handleRenderTemplateDirective"));

    m_executor.submit([this, info]() {
        ACSDK_DEBUG5(LX("handleRenderTemplateDirectiveInExecutor"));
        m_isRenderTemplateLastReceived = true;
        executeDisplayCardEvent(info);
        setHandlingCompleted(info);
    });
}

void TemplateRuntime::handleRenderPlayerInfoDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("handleRenderPlayerInfoDirective"));

    m_executor.submit([this, info]() {
        ACSDK_DEBUG5(LX("handleRenderPlayerInfoDirectiveInExecutor"));
        m_isRenderTemplateLastReceived = false;

        rapidjson::Document payload;
        rapidjson::ParseResult result = payload.Parse(info->directive->getPayload());
        if (!result) {
            ACSDK_ERROR(LX("handleRenderPlayerInfoDirectiveInExecutorParseFailed")
                            .d("reason", rapidjson::GetParseError_En(result.Code()))
                            .d("offset", result.Offset())
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        std::string audioItemId;
        if (!jsonUtils::retrieveValue(payload, AUDIO_ITEM_ID_TAG, &audioItemId)) {
            ACSDK_ERROR(LX("handleRenderPlayerInfoDirective")
                            .d("reason", "missingAudioItemId")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "missing audioItemId");
            return;
        }

        size_t found = std::string::npos;
        for (auto& executionMap : m_audioItemsInExecution) {
            if (!executionMap.second.audioItemId.empty()) {
                found = audioItemId.find(executionMap.second.audioItemId);
            }
            if (found != std::string::npos) {
                ACSDK_DEBUG3(LX("handleRenderPlayerInfoDirectiveInExecutor")
                                 .d("audioItemId", audioItemId)
                                 .m("Matching audioItemId in execution."));
                executionMap.second.directive = info;
                m_activeRenderPlayerInfoCardsProvider = executionMap.first;
                m_audioPlayerInfo[m_activeRenderPlayerInfoCardsProvider].offset =
                    executionMap.first->getAudioItemOffset();
                executeStopTimer();
                executeDisplayCardEvent(info);
                // Since there'a match, we can safely empty m_audioItems.
                m_audioItems.clear();
                break;
            }
        }

        if (std::string::npos == found) {
            ACSDK_DEBUG3(LX("handleRenderPlayerInfoDirectiveInExecutor")
                             .d("audioItemId", audioItemId)
                             .m("Not matching audioItemId in execution."));

            AudioItemPair itemPair{audioItemId, info};
            if (m_audioItems.size() == MAXIMUM_QUEUE_SIZE) {
                // Something is wrong, so we pop the back of the queue and log an error.
                auto discardedAudioItem = m_audioItems.back();
                m_audioItems.pop_back();
                ACSDK_ERROR(LX("handleRenderPlayerInfoDirective")
                                .d("reason", "queueIsFull")
                                .d("discardedAudioItemId", discardedAudioItem.audioItemId));
            }
            m_audioItems.push_front(itemPair);
        }

        setHandlingCompleted(info);
    });
}

void TemplateRuntime::handleUnknownDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_ERROR(LX("handleDirectiveFailed")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    m_executor.submit([this, info] {
        const std::string exceptionMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

        sendExceptionEncounteredAndReportFailed(
            info, exceptionMessage, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    });
}

void TemplateRuntime::executeAudioPlayerInfoUpdates(avsCommon::avs::PlayerActivity state, const Context& context) {
    ACSDK_DEBUG5(LX("executeAudioPlayerInfoUpdates")
                     .d("audioItemId", context.audioItemId)
                     .d("offset", context.offset.count())
                     .d("audioPlayerState", state)
                     .d("isRenderTemplatelastReceived", m_isRenderTemplateLastReceived));

    if (avsCommon::avs::PlayerActivity::IDLE == state || avsCommon::avs::PlayerActivity::BUFFER_UNDERRUN == state) {
        /*
         * The TemplateRuntime Capability Agent is not interested in the IDLE nor BUFFER_UNDERRUN state, so we just
         * ignore the callback.
         */
        return;
    }

    if (!context.mediaProperties) {
        ACSDK_ERROR(LX("executeAudioPlayerInfoUpdatesFailed").d("reason", "nullRenderPlayerInfoCardsInterface"));
        return;
    }

    const auto& currentRenderPlayerInfoCardsProvider = context.mediaProperties;
    if (m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].audioPlayerState == state &&
        m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].audioItemId == context.audioItemId) {
        /*
         * The AudioPlayer notification is chatty during audio playback as it will frequently toggle between
         * BUFFER_UNDERRUN and PLAYER state.  So we filter out the callbacks if the notification are with the
         * same state and audioItemId.
         */
        return;
    }

    auto isStateUpdated = (m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].audioPlayerState != state);
    m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].audioPlayerState = state;
    m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].offset = context.offset;
    if (m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].audioItemId != context.audioItemId) {
        m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].audioItemId = context.audioItemId;
        m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].directive.reset();
        // iterate from front to back (front is most recent)
        for (auto it = m_audioItems.begin(); it != m_audioItems.end(); ++it) {
            auto found = it->audioItemId.find(context.audioItemId);
            if (std::string::npos != found) {
                ACSDK_DEBUG3(LX("executeAudioPlayerInfoUpdates")
                                 .d("audioItemId", context.audioItemId)
                                 .m("Found matching audioItemId in queue."));
                m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].directive = it->directive;
                m_activeRenderPlayerInfoCardsProvider = currentRenderPlayerInfoCardsProvider;
                // We are erasing items older than the current found, as well as the current item.
                m_audioItems.erase(it, m_audioItems.end());
                break;
            }
        }
    }
    if (m_isRenderTemplateLastReceived && state != avsCommon::avs::PlayerActivity::PLAYING) {
        /*
         * If RenderTemplate is the last directive received and the AudioPlayer is not notifying a PLAY,
         * we shouldn't be notifying the observer to render a PlayerInfo display card.
         */
        return;
    }
    m_isRenderTemplateLastReceived = false;

    /*
     * If the AudioPlayer notifies a PLAYING state before the RenderPlayerInfo with the corresponding
     * audioItemId is received, this function will also be called but the m_audioItemsInExecution.directive
     * will be set to nullptr.  So we need to do a nullptr check here to make sure there is a RenderPlayerInfo
     * displayCard to display..
     */
    if (m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].directive) {
        if (isStateUpdated) {
            executeAudioPlayerStartTimer(state);
        }
        executeDisplayCardEvent(m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].directive);
    } else {
        // The RenderTemplateCard is cleared before it's displayed, so we should release the focus.
        if (TemplateRuntime::State::ACQUIRING == m_state) {
            m_state = TemplateRuntime::State::RELEASING;
        }
    }
}

void TemplateRuntime::executeAudioPlayerStartTimer(avsCommon::avs::PlayerActivity state) {
    if (avsCommon::avs::PlayerActivity::PLAYING == state) {
        executeStopTimer();
    } else if (avsCommon::avs::PlayerActivity::PAUSED == state || avsCommon::avs::PlayerActivity::STOPPED == state) {
        executeStartTimer(m_audioPlaybackStoppedPausedTimeout);
    } else if (avsCommon::avs::PlayerActivity::FINISHED == state) {
        executeStartTimer(m_audioPlaybackFinishedTimeout);
    }
}

void TemplateRuntime::executeRenderPlayerInfoCallbacks(bool isClearCard) {
    ACSDK_DEBUG3(LX("executeRenderPlayerInfoCallbacks").d("isClearCard", isClearCard ? "True" : "False"));
    if (isClearCard) {
        for (auto& observer : m_observers) {
            observer->clearPlayerInfoCard();
        }
    } else {
        if (!m_activeRenderPlayerInfoCardsProvider) {
            ACSDK_ERROR(
                LX("executeRenderPlayerInfoCallbacksFailed").d("reason", "nullActiveRenderPlayerInfoCardsProvider"));
            return;
        }
        if (!m_audioItemsInExecution[m_activeRenderPlayerInfoCardsProvider].directive) {
            ACSDK_ERROR(LX("executeRenderPlayerInfoCallbacksFailed").d("reason", "nullAudioItemInExecution"));
            return;
        }
        auto payload =
            m_audioItemsInExecution[m_activeRenderPlayerInfoCardsProvider].directive->directive->getPayload();
        for (auto& observer : m_observers) {
            observer->renderPlayerInfoCard(payload, m_audioPlayerInfo[m_activeRenderPlayerInfoCardsProvider], m_focus);
        }
    }
}

void TemplateRuntime::executeRenderTemplateCallbacks(bool isClearCard) {
    ACSDK_DEBUG3(LX("executeRenderTemplateCallbacks").d("isClear", isClearCard ? "True" : "False"));
    for (auto& observer : m_observers) {
        if (isClearCard) {
            observer->clearTemplateCard();
        } else {
            observer->renderTemplateCard(m_lastDisplayedDirective->directive->getPayload(), m_focus);
        }
    }
}

void TemplateRuntime::executeDisplayCard() {
    if (m_lastDisplayedDirective) {
        if (m_lastDisplayedDirective->directive->getName() == RENDER_TEMPLATE) {
            executeStopTimer();
            executeRenderTemplateCallbacks(false);
        } else {
            executeRenderPlayerInfoCallbacks(false);
        }
    }
}

void TemplateRuntime::executeClearCard() {
    if (m_lastDisplayedDirective) {
        if (m_lastDisplayedDirective->directive->getName() == RENDER_TEMPLATE) {
            executeRenderTemplateCallbacks(true);
        } else {
            executeRenderPlayerInfoCallbacks(true);
        }
    }
}

void TemplateRuntime::executeStartTimer(std::chrono::milliseconds timeout) {
    if (TemplateRuntime::State::DISPLAYING == m_state) {
        ACSDK_DEBUG3(LX("executeStartTimer").d("timeoutInMilliseconds", timeout.count()));
        m_clearDisplayTimer.start(timeout, [this] { m_executor.submit([this] { executeTimerEvent(); }); });
    }
}

void TemplateRuntime::executeStopTimer() {
    ACSDK_DEBUG3(LX("executeStopTimer"));
    m_clearDisplayTimer.stop();
}

/*
 * A state machine is used to acquire and release the visual channel from the visual @c FocusManager.  The state machine
 * has five @c State, and four events as listed below:
 *
 * displayCard - This event happens when the TempateRuntime is ready to notify its observers to display a
 * displayCard.
 *
 * focusChanged - This event happens when the @c FocusManager notifies a change in @c FocusState in the visual
 * channel.
 *
 * timer - This event happens when m_clearDisplayTimer expires and needs to notify its observers to clear the
 * displayCard.
 *
 * cardCleared - This event happens when @c displayCardCleared() is called to notify @c TemplateRuntime the device has
 * cleared the screen.
 *
 * Each state transition may result in one or more of the following actions:
 * (A) Acquire channel
 * (B) Release channel
 * (C) Notify observers to display displayCard
 * (D) Notify observers to clear displayCard
 * (E) Log error about unexpected focusChanged event.
 *
 * Below is the state table illustrating the state transition and its action.  NC means no change in state.
 *
 *                                              E  V  E  N  T  S
 *                -----------------------------------------------------------------------------------------
 *  Current State | displayCard  | timer          | focusChanged::NONE | focusChanged::FG/BG | cardCleared
 * --------------------------------------------------------------------------------------------------------
 * | IDLE         | ACQUIRING(A) | NC             | NC                 | RELEASING(B&E)      | NC
 * | ACQUIRING    | NC           | NC             | IDLE(E)            | DISPLAYING(C)       | NC
 * | DISPLAYING   | NC(C)        | RELEASING(B&D) | IDLE(D)            | DISPLAYING(C)       | RELEASING(B)
 * | RELEASING    | REACQUIRING  | NC             | IDLE               | NC(B&E)             | NC
 * | REACQUIRING  | NC           | NC             | ACQUIRING(A)       | RELEASING(B&E)      | NC
 * --------------------------------------------------------------------------------------------------------
 *
 */

std::string TemplateRuntime::stateToString(const TemplateRuntime::State state) {
    switch (state) {
        case TemplateRuntime::State::IDLE:
            return "IDLE";
        case TemplateRuntime::State::ACQUIRING:
            return "ACQUIRING";
        case TemplateRuntime::State::DISPLAYING:
            return "DISPLAYING";
        case TemplateRuntime::State::RELEASING:
            return "RELEASING";
        case TemplateRuntime::State::REACQUIRING:
            return "REACQUIRING";
    }
    return "UNKNOWN";
}

void TemplateRuntime::executeTimerEvent() {
    State nextState = m_state;

    switch (m_state) {
        case TemplateRuntime::State::DISPLAYING:
            executeClearCard();
            m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
            nextState = TemplateRuntime::State::RELEASING;
            break;

        case TemplateRuntime::State::IDLE:
        case TemplateRuntime::State::ACQUIRING:
        case TemplateRuntime::State::RELEASING:
        case TemplateRuntime::State::REACQUIRING:
            // Do Nothing.
            break;
    }
    ACSDK_DEBUG3(
        LX("executeTimerEvent").d("prevState", stateToString(m_state)).d("nextState", stateToString(nextState)));
    m_state = nextState;
}

void TemplateRuntime::executeOnFocusChangedEvent(avsCommon::avs::FocusState newFocus) {
    ACSDK_DEBUG5(LX("executeOnFocusChangedEvent").d("prevFocus", m_focus).d("newFocus", newFocus));

    bool weirdFocusState = false;
    State nextState = m_state;
    m_focus = newFocus;

    switch (m_state) {
        case TemplateRuntime::State::IDLE:
            // This is weird.  We shouldn't be getting any focus updates in Idle.
            switch (newFocus) {
                case FocusState::FOREGROUND:
                case FocusState::BACKGROUND:
                    weirdFocusState = true;
                    break;
                case FocusState::NONE:
                    // Do nothing.
                    break;
            }
            break;
        case TemplateRuntime::State::ACQUIRING:
            switch (newFocus) {
                case FocusState::FOREGROUND:
                case FocusState::BACKGROUND:
                    executeDisplayCard();
                    nextState = TemplateRuntime::State::DISPLAYING;
                    break;
                case FocusState::NONE:
                    ACSDK_ERROR(LX("executeOnFocusChangedEvent")
                                    .d("prevState", stateToString(m_state))
                                    .d("nextFocus", newFocus)
                                    .m("Unexpected focus state event."));
                    nextState = TemplateRuntime::State::IDLE;
                    break;
            }
            break;
        case TemplateRuntime::State::DISPLAYING:
            switch (newFocus) {
                case FocusState::FOREGROUND:
                case FocusState::BACKGROUND:
                    executeDisplayCard();
                    break;
                case FocusState::NONE:
                    executeClearCard();
                    nextState = TemplateRuntime::State::IDLE;
                    break;
            }
            break;
        case TemplateRuntime::State::RELEASING:
            switch (newFocus) {
                case FocusState::FOREGROUND:
                case FocusState::BACKGROUND:
                    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                    nextState = TemplateRuntime::State::RELEASING;
                    break;
                case FocusState::NONE:
                    nextState = TemplateRuntime::State::IDLE;
                    break;
            }
            break;
        case TemplateRuntime::State::REACQUIRING:
            switch (newFocus) {
                case FocusState::FOREGROUND:
                case FocusState::BACKGROUND:
                    weirdFocusState = true;
                    break;
                case FocusState::NONE:
                    m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), NAMESPACE);
                    nextState = TemplateRuntime::State::ACQUIRING;
                    break;
            }
            break;
    }
    if (weirdFocusState) {
        ACSDK_ERROR(LX("executeOnFocusChangedEvent")
                        .d("prevState", stateToString(m_state))
                        .d("nextFocus", newFocus)
                        .m("Unexpected focus state event."));
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
        nextState = TemplateRuntime::State::RELEASING;
    }
    ACSDK_DEBUG3(LX("executeOnFocusChangedEvent")
                     .d("prevState", stateToString(m_state))
                     .d("nextState", stateToString(nextState)));
    m_state = nextState;
}

void TemplateRuntime::executeDisplayCardEvent(
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    State nextState = m_state;
    m_lastDisplayedDirective = info;

    switch (m_state) {
        case TemplateRuntime::State::IDLE:
            m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), NAMESPACE);
            nextState = TemplateRuntime::State::ACQUIRING;
            break;
        case TemplateRuntime::State::ACQUIRING:
            // Do Nothing.
            break;
        case TemplateRuntime::State::DISPLAYING:
            executeDisplayCard();
            nextState = TemplateRuntime::State::DISPLAYING;
            break;
        case TemplateRuntime::State::RELEASING:
            nextState = TemplateRuntime::State::REACQUIRING;
            break;
        case TemplateRuntime::State::REACQUIRING:
            // Do Nothing.
            break;
    }
    ACSDK_DEBUG3(
        LX("executeDisplayCardEvent").d("prevState", stateToString(m_state)).d("nextState", stateToString(nextState)));
    m_state = nextState;
}

void TemplateRuntime::executeCardClearedEvent() {
    State nextState = m_state;
    switch (m_state) {
        case TemplateRuntime::State::IDLE:
        case TemplateRuntime::State::ACQUIRING:
            // Do Nothing.
            break;
        case TemplateRuntime::State::DISPLAYING:
            m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
            nextState = TemplateRuntime::State::RELEASING;
            break;
        case TemplateRuntime::State::RELEASING:
        case TemplateRuntime::State::REACQUIRING:
            // Do Nothing.
            break;
    }
    ACSDK_DEBUG3(
        LX("executeCardClearedEvent").d("prevState", stateToString(m_state)).d("nextState", stateToString(nextState)));
    m_state = nextState;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> TemplateRuntime::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace templateRuntime
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
