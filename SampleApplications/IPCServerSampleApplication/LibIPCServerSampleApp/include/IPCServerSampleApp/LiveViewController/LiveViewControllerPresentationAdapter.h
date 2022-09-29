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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LIVEVIEWCONTROLLER_LIVEVIEWCONTROLLERPRESENTATIONADAPTER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LIVEVIEWCONTROLLER_LIVEVIEWCONTROLLERPRESENTATIONADAPTER_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerConfiguration.h>
#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>
#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerTypes.h>
#include <acsdk/Notifier/Notifier.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>
#include <acsdk/Sample/InteractionManager/InteractionManager.h>
#include <AIP/ASRProfile.h>
#include <AVSCommon/SDKInterfaces/AudioInputProcessorObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeechInteractionHandlerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <IPCServerSampleApp/AlexaPresentation/AplClientBridge.h>
#include <IPCServerSampleApp/Extensions/LiveView/AplLiveViewExtension.h>
#include <IPCServerSampleApp/Extensions/LiveView/AplLiveViewExtensionObserverInterface.h>
#include <IPCServerSampleApp/IPC/Components/LiveViewCameraHandler.h>
#include <IPCServerSampleApp/IPC/HandlerInterfaces/LiveViewCameraHandlerInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace liveViewController {

/**
 * This class interprets the LiveViewController contract for user interface.
 */
class LiveViewControllerPresentationAdapter
        : public presentationOrchestratorInterfaces::PresentationObserverInterface
        , public ipc::LiveViewCameraHandlerInterface
        , public alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface
        , public avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface
        , public avsCommon::sdkInterfaces::SpeechInteractionHandlerInterface
        , public extensions::liveView::AplLiveViewExtensionObserverInterface
        , public std::enable_shared_from_this<LiveViewControllerPresentationAdapter> {
public:
    /**
     * Create an instance of @c LiveViewControllerPresentationAdapter.
     *
     * @param ipcHandlerRegistrar Pointer to the @c ipc::IPCHandlerRegistrationInterface.
     * @param aplClientBridge Pointer to the @c AplClientBridge to support live view APL extension.
     *
     * @return Shared pointer to the @c LiveViewControllerPresentationAdapter.
     */
    static std::shared_ptr<LiveViewControllerPresentationAdapter> create(
        const std::shared_ptr<ipc::IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
        const std::shared_ptr<AplClientBridge>& aplClientBridge);

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

    /// name LiveViewCameraHandlerInterface methods
    /// @{
    void cameraMicrophoneStateChanged(const std::string& message) override;
    void cameraFirstFrameRendered(const std::string& message) override;
    void windowIdReport(const std::string& message) override;
    /// }

    /// name LiveViewControllerInterface methods
    /// @{
    LiveViewControllerInterface::Response start(
        std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest> request) override;
    LiveViewControllerInterface::Response stop() override;
    LiveViewControllerInterface::Response setCameraState(
        alexaLiveViewControllerInterfaces::CameraState cameraState) override;
    alexaLiveViewControllerInterfaces::Configuration getConfiguration() override;
    bool addObserver(
        std::weak_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerObserverInterface> observer) override;
    void removeObserver(
        std::weak_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerObserverInterface> observer) override;
    /// }

    /// @name AplLiveViewExtensionObserverInterface methods
    /// @{
    void handleCameraExitRequest() override;
    void handleChangeCameraMicStateRequest(bool micOn) override;
    /// }

    /// @name AudioInputProcessorObserverInterface methods
    /// @{
    void onStateChanged(State state) override;
    void onASRProfileChanged(const std::string& profile) override;
    /// }

    /// @name SpeechInteractionHandlerInterface Methods
    /// @{
    std::future<bool> notifyOfWakeWord(
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::string keyword,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) override;

    std::future<bool> notifyOfTapToTalk(
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex = capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now()) override;

    std::future<bool> notifyOfHoldToTalkStart(
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index beginIndex =
            capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX) override;

    std::future<bool> notifyOfHoldToTalkEnd() override;

    std::future<bool> notifyOfTapToTalkEnd() override;
    /// }

    /**
     * Set the presentation orchestrator
     * @param poClient pointer to the presentation orchestrator
     */
    void setPresentationOrchestrator(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface> poClient);

    /**
     * Set the interaction manager
     * @param interactionManager pointer to the interaction manager.
     */
    void setInteractionManager(std::shared_ptr<common::InteractionManager> interactionManager);

    /**
     * Handle setting microphone state for live view camera use cases.
     * @param enabled whether microphone should be turned on for live view camera experiences or not.
     */
    void handleSetCameraMicrophoneState(bool enabled);

private:
    /**
     * Constructor.
     */
    explicit LiveViewControllerPresentationAdapter();

    /**
     * Initializes the @c LiveViewControllerPresentationAdapter.
     *
     * @param ipcHandlerRegistrar Pointer to the @c ipc::IPCHandlerRegistrationInterface.
     * @param aplClientBridge Pointer to the @c AplClientBridge to support live view APL extension.
     */
    void initialize(
        const std::shared_ptr<ipc::IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
        const std::shared_ptr<AplClientBridge>& aplClientBridge);

    /**
     * Handle a change in active @c ASRProfile.
     *
     * @param profile the activce @c ASRProfile
     */
    void handleASRProfileChanged(alexaClientSDK::capabilityAgents::aip::ASRProfile profile);

    /**
     * Dismisses the active presentation.
     */
    void dismissPresentation();

    /**
     * Internal executor for dismissing the active presentation.
     */
    void executeDismissPresentation();

    /**
     * Helper function to notify the observers of the @c LiveViewControllerInterface that live view has been cleared.
     */
    void executeOnPresentationDismissed();

    /**
     * Internal executor for setting the camera UI's mic state
     * @param enabled true if microphone is on, @c false otherwise.
     */
    void executeSetCameraUIMicState(bool enabled);

    /**
     * Helper function to notify the observers of the @c LiveViewControllerInterface that microphone state has been
     * changed.
     *
     * @param enabled @c true if microphone is on, @c false otherwise.
     */
    void executeNotifyMicrophoneStateChanged(bool enabled);

    /// Pointer to the @c PresentationInterface presentation association.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> m_presentation;

    /// Pointer to the presentation orchestrator client
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
        m_presentationOrchestratorClient;

    /// @c AplLiveViewExtensionPtr for handling liveView APL UI interactions.
    extensions::liveView::AplLiveViewExtensionPtr m_aplLiveViewExtension;

    /// The @c LiveViewCameraHandler
    std::shared_ptr<ipc::LiveViewCameraHandler> m_liveViewCameraIPCHandler;

    /// Interaction Manager.
    std::shared_ptr<common::InteractionManager> m_interactionManager;

    /// Active presentation request token for StartLiveView presentation.
    presentationOrchestratorInterfaces::PresentationRequestToken m_startLiveViewRequestToken;

    /// Cached value of the reported live view camera window id.
    std::string m_liveViewCameraWindowId;

    /// Pointer to the @c StartLiveViewRequest.
    std::unique_ptr<LiveViewControllerInterface::StartLiveViewRequest> m_startLiveViewRequest;

    /// Shared pointer to the active @c ASRProfile
    alexaClientSDK::capabilityAgents::aip::ASRProfile m_asrProfile;

    /// Shared pointer to the executor.
    avsCommon::utils::threading::Executor m_executor;

    /// The notifier of @c LiveViewControllerInterface observers.
    notifier::Notifier<alexaLiveViewControllerInterfaces::LiveViewControllerObserverInterface> m_notifier;
};

}  // namespace liveViewController
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LIVEVIEWCONTROLLER_LIVEVIEWCONTROLLERPRESENTATIONADAPTER_H_
