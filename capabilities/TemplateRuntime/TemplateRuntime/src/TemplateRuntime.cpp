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

#include <algorithm>
#include <utility>

#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Memory/Memory.h"

#include "acsdk/TemplateRuntime/private/TemplateRuntime.h"

namespace alexaClientSDK {
namespace templateRuntime {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;
using namespace templateRuntimeInterfaces;

/// TemplateRuntime capability constants
/// TemplateRuntime interface type
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// TemplateRuntime interface name
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_NAME = "TemplateRuntime";
/// TemplateRuntime interface version
static const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_VERSION = "1.1";

/// String to identify log entries originating from this file.
#define TAG "TemplateRuntime"

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

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Creates the TemplateRuntime capability configuration.
 *
 * @return The TemplateRuntime capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getTemplateRuntimeCapabilityConfiguration();

std::shared_ptr<TemplateRuntime> TemplateRuntime::create(
    const std::unordered_set<std::shared_ptr<RenderPlayerInfoCardsProviderInterface>>& renderPlayerInfoCardInterface,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }
    std::shared_ptr<TemplateRuntime> templateRuntime(
        new TemplateRuntime(renderPlayerInfoCardInterface, exceptionSender));

    for (const auto& renderPlayerInfoCardProvider : renderPlayerInfoCardInterface) {
        if (!renderPlayerInfoCardProvider) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullRenderPlayerInfoCardInterface"));
            return nullptr;
        }
        renderPlayerInfoCardProvider->setObserver(templateRuntime);
    }

    return templateRuntime;
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

void TemplateRuntime::onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity state, const Context& context) {
    ACSDK_DEBUG5(LX("onRenderPlayerCardsInfoChanged"));
    m_executor.submit([this, state, context]() {
        ACSDK_DEBUG5(LX("onPlayerActivityChangedInExecutor"));
        executeAudioPlayerInfoUpdates(state, context);
    });
}

void TemplateRuntime::addObserver(std::weak_ptr<TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG5(LX("addObserver"));
    m_notifier->addWeakPtrObserver(observer);
}

void TemplateRuntime::removeObserver(std::weak_ptr<TemplateRuntimeObserverInterface> observer) {
    ACSDK_DEBUG5(LX("removeObserver"));
    m_notifier->removeWeakPtrObserver(observer);
}

TemplateRuntime::TemplateRuntime(
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>
        renderPlayerInfoCardsInterfaces,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, std::move(exceptionSender)},
        RequiresShutdown{"TemplateRuntime"},
        m_isRenderTemplateLastReceived{false},
        m_renderPlayerInfoCardsInterfaces{std::move(renderPlayerInfoCardsInterfaces)} {
    m_capabilityConfigurations.insert(getTemplateRuntimeCapabilityConfiguration());
    m_notifier = std::make_shared<TemplateRuntimeNotifier>();
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
    m_observers.clear();
    m_activeRenderPlayerInfoCardsProvider.reset();
    m_audioItemsInExecution.clear();
    m_audioPlayerInfo.clear();
    for (const auto& renderPlayerInfoCardsInterface : m_renderPlayerInfoCardsInterfaces) {
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

void TemplateRuntime::setHandlingCompleted(const std::shared_ptr<DirectiveInfo>& info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void TemplateRuntime::handleRenderTemplateDirective(const std::shared_ptr<DirectiveInfo>& info) {
    ACSDK_DEBUG5(LX("handleRenderTemplateDirective"));

    m_executor.submit([this, info]() {
        ACSDK_DEBUG5(LX("handleRenderTemplateDirectiveInExecutor"));
        m_isRenderTemplateLastReceived = true;
        executeDisplayCard(info);
        setHandlingCompleted(info);
    });
}

void TemplateRuntime::handleRenderPlayerInfoDirective(const std::shared_ptr<DirectiveInfo>& info) {
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
                m_audioPlayerInfo[m_activeRenderPlayerInfoCardsProvider].mediaProperties = executionMap.first;
                executeDisplayCard(info);
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

    m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].audioPlayerState = state;
    m_audioPlayerInfo[currentRenderPlayerInfoCardsProvider].mediaProperties = context.mediaProperties;
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
        executeDisplayCard(m_audioItemsInExecution[currentRenderPlayerInfoCardsProvider].directive);
    }
}

void TemplateRuntime::executeRenderPlayerInfoCallback() {
    ACSDK_DEBUG3(LX("executeRenderPlayerInfoCallback"));
    if (!m_activeRenderPlayerInfoCardsProvider) {
        ACSDK_ERROR(LX("executeRenderPlayerInfoCallbackFailed").d("reason", "nullActiveRenderPlayerInfoCardsProvider"));
        return;
    }
    if (!m_audioItemsInExecution[m_activeRenderPlayerInfoCardsProvider].directive) {
        ACSDK_ERROR(LX("executeRenderPlayerInfoCallbackFailed").d("reason", "nullAudioItemInExecution"));
        return;
    }
    auto payload = m_audioItemsInExecution[m_activeRenderPlayerInfoCardsProvider].directive->directive->getPayload();
    m_notifier->notifyObservers([this, payload](const std::shared_ptr<TemplateRuntimeObserverInterface>& observer) {
        if (observer) {
            observer->renderPlayerInfoCard(payload, m_audioPlayerInfo[m_activeRenderPlayerInfoCardsProvider]);
        }
    });
}

void TemplateRuntime::executeRenderTemplateCallback() {
    ACSDK_DEBUG3(LX("executeRenderTemplateCallback"));
    m_notifier->notifyObservers([this](const std::shared_ptr<TemplateRuntimeObserverInterface>& observer) {
        if (observer) {
            observer->renderTemplateCard(m_lastDisplayedDirective->directive->getPayload());
        }
    });
}

void TemplateRuntime::executeDisplayCard(std::shared_ptr<DirectiveInfo> info) {
    m_lastDisplayedDirective = std::move(info);

    if (m_lastDisplayedDirective) {
        if (m_lastDisplayedDirective->directive->getName() == RENDER_TEMPLATE) {
            executeRenderTemplateCallback();
        } else {
            executeRenderPlayerInfoCallback();
        }
    }
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> TemplateRuntime::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void TemplateRuntime::addRenderPlayerInfoCardsProvider(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface> cardsProvider) {
    ACSDK_DEBUG5(LX("addRenderPlayerInfoCardsProvider"));

    if (!cardsProvider) {
        ACSDK_ERROR(
            LX("addRenderPlayerInfoCardsProviderFailed").d("reason", "nullRenderPlayerInfoCardsProviderInterface"));
        return;
    }
    cardsProvider->setObserver(shared_from_this());
    m_renderPlayerInfoCardsInterfaces.insert(cardsProvider);
}

}  // namespace templateRuntime
}  // namespace alexaClientSDK
