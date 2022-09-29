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

#include "acsdk/Sample/TemplateRuntime/TemplateRuntimePresentationAdapter.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace avsCommon::avs;
using namespace avsCommon::utils::configuration;
using namespace acsdkAudioPlayerInterfaces;
using namespace presentationOrchestratorInterfaces;

/// String to identify log entries originating from this file.
#define TAG "TemplateRuntimePresentationAdapter"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Interface name for Alexa.TemplateRuntime requests.
static const char TEMPLATE_RUNTIME_INTERFACE_NAME[] = "TemplateRuntime";

/// Request token for RenderTemplate directive.
static const char RENDER_TEMPLATE_TOKEN[] = "RenderTemplate";

/// Request token for PlayerInfo directive.
static const char PLAYER_INFO_TOKEN[] = "RenderPlayerInfo";

/// The key in our config file to find the root of template runtime configuration.
static const char TEMPLATERUNTIME_CONFIGURATION_ROOT_KEY[] = "templateRuntimeCapabilityAgent";

/// The key in our config file to set the display card timeout value when AudioPlayer is in FINISHED state
static const char TEMPLATERUNTIME_AUDIOPLAYBACK_FINISHED_KEY[] = "displayCardAudioPlaybackFinishedTimeout";

/// The key in our config file to set the display card timeout value when AudioPlayer is in STOPPED or PAUSED state
static const char TEMPLATERUNTIME_AUDIOPLAYBACK_STOPPED_PAUSED_KEY[] = "displayCardAudioPlaybackStoppedPausedTimeout";

/// Default timeout for clearing the RenderPlayerInfo display card when AudioPlayer is in FINISHED state.
static const std::chrono::milliseconds DEFAULT_AUDIO_FINISHED_TIMEOUT_MS{10000};

/// Default timeout for clearing the RenderPlayerInfo display card when AudioPlayer is in STOPPED/PAUSED state.
static const std::chrono::milliseconds DEFAULT_AUDIO_STOPPED_PAUSED_TIMEOUT_MS{60000};

std::shared_ptr<TemplateRuntimePresentationAdapter> TemplateRuntimePresentationAdapter::create() {
    auto templateRuntimePresentationAdapter =
        std::shared_ptr<TemplateRuntimePresentationAdapter>(new TemplateRuntimePresentationAdapter());

    templateRuntimePresentationAdapter->initialize();

    return templateRuntimePresentationAdapter;
}

TemplateRuntimePresentationAdapter::TemplateRuntimePresentationAdapter() :
        m_executor(std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>()) {
    m_templateRuntimePresentationAdapterNotifier = std::make_shared<TemplateRuntimePresentationAdapterNotifier>();
}

bool TemplateRuntimePresentationAdapter::initialize() {
    auto configurationRoot = ConfigurationNode::getRoot()[std::string(TEMPLATERUNTIME_CONFIGURATION_ROOT_KEY)];

    configurationRoot.getDuration<std::chrono::milliseconds>(
        TEMPLATERUNTIME_AUDIOPLAYBACK_FINISHED_KEY, &m_audioPlaybackFinishedTimeout, DEFAULT_AUDIO_FINISHED_TIMEOUT_MS);

    configurationRoot.getDuration<std::chrono::milliseconds>(
        TEMPLATERUNTIME_AUDIOPLAYBACK_STOPPED_PAUSED_KEY,
        &m_audioPlaybackStoppedPausedTimeout,
        DEFAULT_AUDIO_STOPPED_PAUSED_TIMEOUT_MS);

    return true;
}

void TemplateRuntimePresentationAdapter::addTemplateRuntimePresentationAdapterObserver(
    const std::weak_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit(
        [this, observer]() { m_templateRuntimePresentationAdapterNotifier->addWeakPtrObserver(observer); });
}

void TemplateRuntimePresentationAdapter::removeTemplateRuntimePresentationAdapterObserver(
    const std::weak_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit(
        [this, observer]() { m_templateRuntimePresentationAdapterNotifier->removeWeakPtrObserver(observer); });
}

void TemplateRuntimePresentationAdapter::onPresentationAvailable(
    PresentationRequestToken id,
    std::shared_ptr<PresentationInterface> presentation) {
    ACSDK_DEBUG5(LX(__func__).d("id", id));

    m_executor->submit([this, id, presentation]() {
        if (id == m_playerInfoRequestToken && m_playerInfoCardSession) {
            m_playerInfoCardSession->presentation = presentation;
            m_playerInfoCardSession->presentationState = presentation->getState();
            executeOnPlayerActivityChanged(m_playerInfoCardSession->audioPlayerInfo.audioPlayerState);
            m_templateRuntimePresentationAdapterNotifier->notifyObservers(
                [this](const std::shared_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
                    observer->renderPlayerInfoCard(
                        m_playerInfoCardSession->jsonPayload, m_playerInfoCardSession->audioPlayerInfo);
                });
        } else if (id == m_renderTemplateRequestToken && m_displayCardSession) {
            m_displayCardSession->presentation = presentation;
            m_displayCardSession->presentationState = presentation->getState();
            m_templateRuntimePresentationAdapterNotifier->notifyObservers(
                [this](const std::shared_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
                    observer->renderTemplateCard(m_displayCardSession->jsonPayload);
                });
        }
    });
}

void TemplateRuntimePresentationAdapter::onPresentationStateChanged(
    PresentationRequestToken id,
    PresentationState newState) {
    ACSDK_DEBUG5(LX(__func__).d("id", id).d("newState", newState));

    m_executor->submit([this, id, newState] {
        DisplayCardSessionPtr session = nullptr;
        if (id == m_playerInfoRequestToken && m_playerInfoCardSession) {
            session = m_playerInfoCardSession;
        } else if (id == m_renderTemplateRequestToken && m_displayCardSession) {
            session = m_displayCardSession;
        }
        if (!session || newState == session->presentationState) {
            return;
        }
        session->presentationState = newState;
        auto& presentation = session->presentation;

        if (PresentationState::NONE == newState) {
            if (id == m_playerInfoRequestToken) {
                m_templateRuntimePresentationAdapterNotifier->notifyObservers(
                    [](const std::shared_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
                        observer->clearPlayerInfoCard();
                    });
            } else if (id == m_renderTemplateRequestToken) {
                m_templateRuntimePresentationAdapterNotifier->notifyObservers(
                    [](const std::shared_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
                        observer->clearRenderTemplateCard();
                    });
            }
            if (presentation) {
                presentation.reset();
            }
        }
    });
}

bool TemplateRuntimePresentationAdapter::onNavigateBack(PresentationRequestToken id) {
    /* NO OP: Let PO manage the back navigation */
    return false;
}

void TemplateRuntimePresentationAdapter::renderTemplateCard(const std::string& jsonPayload) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor->submit([this, jsonPayload]() {
        PresentationOptions poOptions;
        poOptions.presentationLifespan = PresentationLifespan::TRANSIENT;
        poOptions.metadata = RENDER_TEMPLATE_TOKEN;
        poOptions.interfaceName = TEMPLATE_RUNTIME_INTERFACE_NAME;
        poOptions.timeout = PresentationInterface::getTimeoutDefault();

        m_displayCardSession = std::make_shared<DisplayCardSession>(jsonPayload);

        m_renderTemplateRequestToken =
            m_presentationOrchestratorClient->requestWindow(m_renderTemplateWindowId, poOptions, shared_from_this());
    });
}

void TemplateRuntimePresentationAdapter::renderPlayerInfoCard(
    const std::string& jsonPayload,
    alexaClientSDK::templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor->submit([this, jsonPayload, audioPlayerInfo]() {
        if (m_playerInfoCardSession && m_playerInfoCardSession->presentation &&
            PresentationState::NONE != m_playerInfoCardSession->presentationState) {
            m_playerInfoCardSession->jsonPayload = jsonPayload;
            m_playerInfoCardSession->audioPlayerInfo = audioPlayerInfo;
            if (PresentationState::FOREGROUND == m_playerInfoCardSession->presentationState) {
                m_templateRuntimePresentationAdapterNotifier->notifyObservers(
                    [this](const std::shared_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer) {
                        observer->renderPlayerInfoCard(
                            m_playerInfoCardSession->jsonPayload, m_playerInfoCardSession->audioPlayerInfo);
                    });
            }
            return;
        }
        PresentationOptions poOptions;
        poOptions.presentationLifespan = PresentationLifespan::LONG;
        poOptions.metadata = PLAYER_INFO_TOKEN;
        poOptions.interfaceName = TEMPLATE_RUNTIME_INTERFACE_NAME;
        poOptions.timeout = PresentationInterface::getTimeoutDefault();

        m_playerInfoCardSession = std::make_shared<PlayerInfoCardSession>(jsonPayload, audioPlayerInfo);

        m_playerInfoRequestToken =
            m_presentationOrchestratorClient->requestWindow(m_renderPlayerInfoWindowId, poOptions, shared_from_this());
    });
}

void TemplateRuntimePresentationAdapter::setRenderTemplateWindowId(const std::string& renderTemplateWindowId) {
    m_executor->submit([this, renderTemplateWindowId]() { m_renderTemplateWindowId = renderTemplateWindowId; });
}

void TemplateRuntimePresentationAdapter::setRenderPlayerInfoWindowId(const std::string& renderPlayerInfoWindowId) {
    m_executor->submit([this, renderPlayerInfoWindowId]() { m_renderPlayerInfoWindowId = renderPlayerInfoWindowId; });
}

void TemplateRuntimePresentationAdapter::setExecutor(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> executor) {
    m_executor = std::move(executor);
}

void TemplateRuntimePresentationAdapter::onPlayerActivityChanged(
    PlayerActivity state,
    const AudioPlayerObserverInterface::Context& context) {
    ACSDK_DEBUG9(LX(__func__).d("newState", playerActivityToString(state).c_str()));
    m_executor->submit([this, state]() { executeOnPlayerActivityChanged(state); });
}

void TemplateRuntimePresentationAdapter::executeOnPlayerActivityChanged(PlayerActivity state) {
    if (!m_playerInfoCardSession || !m_playerInfoCardSession->presentation) {
        return;
    }
    std::chrono::milliseconds presentationTimeout;
    PresentationLifespan presentationLifespan;
    switch (state) {
        case PlayerActivity::PLAYING:
        case PlayerActivity::BUFFER_UNDERRUN:
            presentationTimeout = m_playerInfoCardSession->presentation->getTimeoutDisabled();
            presentationLifespan = PresentationLifespan::LONG;
            break;
        case PlayerActivity::IDLE:
        case PlayerActivity::STOPPED:
        case PlayerActivity::PAUSED:
            presentationTimeout = m_audioPlaybackStoppedPausedTimeout;
            presentationLifespan = PresentationLifespan::TRANSIENT;
            break;
        case PlayerActivity::FINISHED:
            presentationTimeout = m_audioPlaybackFinishedTimeout;
            presentationLifespan = PresentationLifespan::SHORT;
            break;
        default:
            /* Do Nothing */
            return;
    }

    m_playerInfoCardSession->presentation->setLifespan(presentationLifespan);
    m_playerInfoCardSession->presentation->setTimeout(presentationTimeout);
    m_playerInfoCardSession->presentation->startTimeout();
}

void TemplateRuntimePresentationAdapter::setPresentationOrchestrator(
    std::shared_ptr<PresentationOrchestratorClientInterface> poClient) {
    m_presentationOrchestratorClient = std::move(poClient);
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
