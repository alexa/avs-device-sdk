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

#include <rapidjson/error/en.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "TemplateRuntime/TemplateRuntime.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace templateRuntime {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
static const std::string TAG{"TemplateRuntime"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"TemplateRuntime"};

/// The RenderTemplate directive signature.
static const NamespaceAndName TEMPLATE{NAMESPACE, "RenderTemplate"};

/// The RenderPlayerInfo directive signature.
static const NamespaceAndName PLAYER_INFO{NAMESPACE, "RenderPlayerInfo"};

/// Tag for find the AudioItemId in the payload of the RenderPlayerInfo directive
static const std::string AUDIO_ITEM_ID_TAG{"audioItemId"};

/// Maximum queue size allowed for m_audioItems.
static const size_t MAXIMUM_QUEUE_SIZE{100};

std::shared_ptr<TemplateRuntime> TemplateRuntime::create(
    std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerInterface> audioPlayerInterface,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender) {
    if (!audioPlayerInterface) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAudioPlayerInterface"));
        return nullptr;
    }

    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }
    std::shared_ptr<TemplateRuntime> templateRuntime(new TemplateRuntime(audioPlayerInterface, exceptionSender));
    audioPlayerInterface->addObserver(templateRuntime);
    return templateRuntime;
}

void TemplateRuntime::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX("handleDirectiveImmediately"));
    preHandleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void TemplateRuntime::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("preHandleDirective"));
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

void TemplateRuntime::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleDirective"));
    // Do nothing here as directives are handled in the preHandle stage.
}

void TemplateRuntime::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

DirectiveHandlerConfiguration TemplateRuntime::getConfiguration() const {
    ACSDK_DEBUG9(LX("getConfiguration"));
    DirectiveHandlerConfiguration configuration;
    configuration[TEMPLATE] = BlockingPolicy::HANDLE_IMMEDIATELY;
    configuration[PLAYER_INFO] = BlockingPolicy::HANDLE_IMMEDIATELY;
    return configuration;
}

void TemplateRuntime::onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) {
    ACSDK_DEBUG9(LX("onPlayerActivityChanged"));
    m_executor.submit([this, state, context]() {
        ACSDK_DEBUG0(LX("onPlayerActivityChangedInExecutor"));
        executeAudioPlayerInfoUpdates(state, context);
    });
}

void TemplateRuntime::addObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG9(LX("addObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("addObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer]() {
        ACSDK_DEBUG0(LX("addObserverInExecutor"));
        if (!m_observers.insert(observer).second) {
            ACSDK_ERROR(LX("addObserverInExecutor").m("Duplicate observer."));
        }
    });
}

void TemplateRuntime::removeObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG9(LX("removeObserver"));
    if (!observer) {
        ACSDK_ERROR(LX("removeObserver").m("Observer is null."));
        return;
    }
    m_executor.submit([this, observer]() {
        ACSDK_DEBUG0(LX("removeObserverInExecutor"));
        if (m_observers.erase(observer) == 0) {
            ACSDK_WARN(LX("removeObserverInExecutor").m("Nonexistent observer."));
        }
    });
}

TemplateRuntime::TemplateRuntime(
    std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerInterface> audioPlayerInterface,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"TemplateRuntime"},
        m_isRenderTemplateLastReceived{false},
        m_audioPlayerInterface{audioPlayerInterface} {
}

void TemplateRuntime::doShutdown() {
    m_executor.shutdown();
    m_observers.clear();
    m_audioPlayerInterface->removeObserver(shared_from_this());
    m_audioPlayerInterface.reset();
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

void TemplateRuntime::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void TemplateRuntime::handleRenderTemplateDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleRenderTemplateDirective"));

    m_executor.submit([this, info]() {
        ACSDK_DEBUG0(LX("handleRenderTemplateDirectiveInExecutor"));
        m_isRenderTemplateLastReceived = true;
        for (auto observer : m_observers) {
            observer->renderTemplateCard(info->directive->getPayload());
        }
        setHandlingCompleted(info);
    });
}

void TemplateRuntime::handleRenderPlayerInfoDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleRenderPlayerInfoDirective"));

    m_executor.submit([this, info]() {
        ACSDK_DEBUG0(LX("handleRenderPlayerInfoDirectiveInExecutor"));
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

        if (m_audioItemInExecution.audioItemId != audioItemId) {
            ACSDK_DEBUG0(LX("handleRenderPlayerInfoDirectiveInExecutor")
                             .d("audioItemId", audioItemId)
                             .m("Not matching audioItemId in execution."));
            AudioItemPair itemPair{audioItemId, info};
            if (m_audioItems.size() == MAXIMUM_QUEUE_SIZE) {
                // Something is wrong, so we pop the front of the queue and log an error.
                auto discardedAudioItem = m_audioItems.front();
                m_audioItems.pop();
                ACSDK_ERROR(LX("handleRenderPlayerInfoDirective")
                                .d("reason", "queueIsFull")
                                .d("discardedAudioItemId", discardedAudioItem.audioItemId));
            }
            m_audioItems.push(itemPair);
        } else {
            ACSDK_DEBUG0(LX("handleRenderPlayerInfoDirectiveInExecutor")
                             .d("audioItemId", audioItemId)
                             .m("Matching audioItemId in execution."));
            m_audioItemInExecution.directive = info;
            m_audioPlayerInfo.offset = m_audioPlayerInterface->getAudioItemOffset();
            executeRenderPlayerInfoCallbacks();
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
    ACSDK_DEBUG0(LX("executeAudioPlayerInfoUpdates")
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

    if (m_audioPlayerInfo.audioPlayerState == state && m_audioItemInExecution.audioItemId == context.audioItemId) {
        /*
         * The AudioPlayer notification is chatty during audio playback as it will frequently toggle between
         * BUFFER_UNDERRUN and PLAYER state.  So we filter out the callbacks if the notification are with the
         * same state and audioItemId.
         */
        return;
    }

    m_audioPlayerInfo.audioPlayerState = state;
    m_audioPlayerInfo.offset = context.offset;
    if (m_audioItemInExecution.audioItemId != context.audioItemId) {
        m_audioItemInExecution.audioItemId = context.audioItemId;
        m_audioItemInExecution.directive.reset();
        while (!m_audioItems.empty()) {
            auto audioItem = m_audioItems.front();
            m_audioItems.pop();
            if (audioItem.audioItemId == context.audioItemId) {
                ACSDK_DEBUG0(LX("executeAudioPlayerInfoUpdates")
                                 .d("audioItemId", context.audioItemId)
                                 .m("Found matching audioItemId in queue."));
                m_audioItemInExecution.directive = audioItem.directive;
                break;
            } else {
                ACSDK_DEBUG0(LX("executeAudioPlayerInfoUpdates")
                                 .d("audioItemId", audioItem.audioItemId)
                                 .m("Dropping out-dated audioItemId in queue."));
            }
        }
    }
    if (m_isRenderTemplateLastReceived && state != avsCommon::avs::PlayerActivity::PLAYING) {
        /*
         * If RenderTemplate is the last directive received and the AudioPlayer is not notifying a PLAY,
         * we shouldn't be notifing the observer to render a PlayerInfo display card.
         */
        return;
    }
    m_isRenderTemplateLastReceived = false;

    /*
     * If the AudioPlayer notifies a PLAYING state before the RenderPlayerInfo with the corresponding
     * audioItemId is received, this function will also be called but the m_audioItemInExecution.directive
     * will be set to nullptr and the callback to the observers will be filtered out by the nullptr
     * check in executeRenderPlayerInfoCallbacks().
     */
    executeRenderPlayerInfoCallbacks();
}

void TemplateRuntime::executeRenderPlayerInfoCallbacks() {
    ACSDK_DEBUG0(LX("executeRenderPlayerInfoCallbacks"));
    if (m_audioItemInExecution.directive) {
        for (auto& observer : m_observers) {
            observer->renderPlayerInfoCard(
                m_audioItemInExecution.directive->directive->getPayload(), m_audioPlayerInfo);
        }
    }
}

}  // namespace templateRuntime
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
