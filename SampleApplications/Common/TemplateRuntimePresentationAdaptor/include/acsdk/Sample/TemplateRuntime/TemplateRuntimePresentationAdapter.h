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

#ifndef ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTER_H_
#define ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTER_H_

#include <string>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h>
#include <acsdk/Notifier/Notifier.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>
#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "TemplateRuntimePresentationAdapterObserverInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * This class interprets the Template Runtime contract for user interface.
 */
class TemplateRuntimePresentationAdapter
        : public presentationOrchestratorInterfaces::PresentationObserverInterface
        , public templateRuntimeInterfaces::TemplateRuntimeObserverInterface
        , public acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface
        , public std::enable_shared_from_this<TemplateRuntimePresentationAdapter> {
public:
    /**
     * Create an instance of @c TemplateRuntimePresentationAdapter
     * @return Shared pointer to the @c TemplateRuntimePresentationAdapter
     */
    static std::shared_ptr<TemplateRuntimePresentationAdapter> create();

    /**
     * Destructor
     */
    virtual ~TemplateRuntimePresentationAdapter() = default;

    /// @name PresentationObserverInterface functions
    /// @{
    void onPresentationAvailable(
        presentationOrchestratorInterfaces::PresentationRequestToken id,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> presentation) override;
    void onPresentationStateChanged(
        presentationOrchestratorInterfaces::PresentationRequestToken id,
        presentationOrchestratorInterfaces::PresentationState newState) override;
    bool onNavigateBack(presentationOrchestratorInterfaces::PresentationRequestToken id) override;
    /// @}

    /// @name TemplateRuntimeObserverInterface functions
    /// @{
    void renderTemplateCard(const std::string& jsonPayload) override;

    void renderPlayerInfoCard(
        const std::string& jsonPayload,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) override;

    /// @}

    /// @name AudioPlayerObserverInterface Functions
    /// @{
    void onPlayerActivityChanged(
        avsCommon::avs::PlayerActivity state,
        const acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface::Context& context) override;
    /// @}

    /**
     * Set the executor used as the worker thread
     * @param executor The @c Executor to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setExecutor(std::shared_ptr<avsCommon::utils::threading::Executor> executor);

    /**
     * Registers an observer to the adapter for presentation changes.
     * @param observer Instance of an observer to be notified of any change.
     */
    void addTemplateRuntimePresentationAdapterObserver(
        const std::weak_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer);

    /**
     * De-registers an observer from the adapter for presentation changes.
     * @param observer Instance of an observer to be removed.
     */
    void removeTemplateRuntimePresentationAdapterObserver(
        const std::weak_ptr<TemplateRuntimePresentationAdapterObserverInterface>& observer);

    /**
     * Set the window id to use for render template card presentations managed by the adapter
     * @param renderTemplateWindowId id of the render template window
     */
    void setRenderTemplateWindowId(const std::string& renderTemplateWindowId);

    /**
     * Set the window id to use for render player info card presentations managed by the adapter
     * @param renderPlayerInfoWindowId id of the render player info window
     */
    void setRenderPlayerInfoWindowId(const std::string& renderPlayerInfoWindowId);

    /**
     * Set the presentation orchestrator
     * @param poClient pointer to the presentation orchestrator
     */
    void setPresentationOrchestrator(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface> poClient);

private:
    /**
     * Constructor
     */
    explicit TemplateRuntimePresentationAdapter();

    /**
     * Initializes the @c TemplateRuntimePresentationAdapter object.
     */
    bool initialize();

    /// Alias for Notifier based on @c TemplateRuntimePresentationAdapterObserverInterface.
    using TemplateRuntimePresentationAdapterNotifier =
        notifier::Notifier<TemplateRuntimePresentationAdapterObserverInterface>;

    /**
     * Session for DisplayCard Presentation
     */
    struct DisplayCardSession {
        /**
         * Default Constructor.
         */
        DisplayCardSession() = default;

        /**
         * Constructor
         * @param jsonPayload DisplayCard json payload;
         */
        explicit DisplayCardSession(std::string jsonPayload) :
                jsonPayload{std::move(jsonPayload)},
                presentation{nullptr},
                presentationState{presentationOrchestratorInterfaces::PresentationState::NONE} {};

        /// The json payload for the DisplayCard session
        std::string jsonPayload;

        /// Pointer to the @c PresentationInterface presentation association for this session
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> presentation;

        /// The @c PresentationState of this session
        presentationOrchestratorInterfaces::PresentationState presentationState;
    };

    /// Convenience typedef
    using DisplayCardSessionPtr = std::shared_ptr<DisplayCardSession>;

    /**
     * Session for PlayerInfo Presentation
     */
    struct PlayerInfoCardSession : DisplayCardSession {
        /**
         * Default Constructor.
         */
        PlayerInfoCardSession() = default;

        /**
         * Constructor
         * @param jsonPayload playerInfo json payload
         * @param audioPlayerInfo associated audioPlayerInfo
         */
        PlayerInfoCardSession(
            std::string jsonPayload,
            templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) :
                jsonPayload{std::move(jsonPayload)},
                audioPlayerInfo{std::move(audioPlayerInfo)} {};

        /// The json payload for the playerInfo session
        std::string jsonPayload;

        /// The @c AudioPlayerInfo for this session
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo;
    };

    /// Convenience typedef
    using PlayerInfoCardSessionPtr = std::shared_ptr<PlayerInfoCardSession>;

    /**
     * In executor method to process player activity updates.
     * @param state Player state.
     */
    void executeOnPlayerActivityChanged(alexaClientSDK::avsCommon::avs::PlayerActivity state);

    /// Shared pointer to the template runtime presentation adapter notifier.
    std::shared_ptr<TemplateRuntimePresentationAdapterNotifier> m_templateRuntimePresentationAdapterNotifier;

    /// Pointer to the presentation orchestrator client
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
        m_presentationOrchestratorClient;

    /// Active presentation request token for RenderTemplate presentation
    presentationOrchestratorInterfaces::PresentationRequestToken m_renderTemplateRequestToken;

    /// Active presentation request token for PlayerInfo presentation
    presentationOrchestratorInterfaces::PresentationRequestToken m_playerInfoRequestToken;

    /// Pointer to DisplayCard Session
    DisplayCardSessionPtr m_displayCardSession;

    /// Pointer to PlayerInfoCard Session
    PlayerInfoCardSessionPtr m_playerInfoCardSession;

    /// The timeout value in ms for clearing the display card when AudioPlay is FINISHED
    std::chrono::milliseconds m_audioPlaybackFinishedTimeout;

    /// The timeout value in ms for clearing the display card when AudioPlay is STOPPED or PAUSED
    std::chrono::milliseconds m_audioPlaybackStoppedPausedTimeout;

    /// Id of the window used to handle render template payloads.
    std::string m_renderTemplateWindowId;

    /// Id of the window used to handle render player info payloads.
    std::string m_renderPlayerInfoWindowId;

    /// Shared pointer to the executor.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTER_H_
