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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_

#include <list>
#include <utility>

#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <AIP/AudioInputProcessor.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <CertifiedSender/CertifiedSender.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <SoftwareComponentReporter/SoftwareComponentReporterCapabilityAgent.h>
#include <System/ReportStateHandler.h>
#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeInterface.h>

namespace alexaClientSDK {
namespace defaultClient {
/**
 * This interface provides a facility to add external capabilities to default client.
 *
 * Default client will provide its core components in the build call.
 *
 * @note Any object created during the buildCapabilities that keep a pointer to a core component should be added or
 * managed by an object in the list of @c RequiresShutdown objects returned by buildCapabilities(). This will ensure
 * that these objects are shutdown during @c DefaultClient shutdown and before any core component is shutdown.
 */
class ExternalCapabilitiesBuilderInterface {
public:
    /**
     * Define a capability structure.
     */
    struct Capability {
        /// The optional capability configuration which will be included in the list of supported capabilities sent
        /// to AVS.
        avsCommon::utils::Optional<avsCommon::avs::CapabilityConfiguration> configuration;

        /// An optional directive handler used to process any directive included in this capability.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
    };

    /**
     * Destructor.
     */
    virtual ~ExternalCapabilitiesBuilderInterface() = default;

    /**
     * This method sets the storage using for setting.
     *
     * @warning The settings storage is opened and closed by DefaultClient during creation and shutdown respectively.
     * You can add your objects that use this storage inside the list of RequiresShutdown objects managed by
     * DefaultClient to ensure the storage usage will not be done after it closure.
     */
    virtual ExternalCapabilitiesBuilderInterface& withSettingsStorage(
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingStorage) = 0;

    /**
     * This method sets the TemplateRuntime Capability Agent for visual interactions.
     *
     * This method will only get called if GUI supports has been enabled.
     *
     * @param templateRuntime The TemplateRuntime object.
     */
    virtual ExternalCapabilitiesBuilderInterface& withTemplateRunTime(
        std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeInterface> templateRuntime) = 0;

    /**
     * Get the CallManager reference.
     *
     * @return A reference of CallManager.
     */
    virtual std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CallManagerInterface> getCallManager() = 0;

    /**
     * This method sets the InternetConnectionMonitor for CallManager.
     *
     * @param internetConnectionMonitor The InternetConnectionMonitor object.
     */
    virtual ExternalCapabilitiesBuilderInterface& withInternetConnectionMonitor(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> internetConnectionMonitor) = 0;

    /**
     * This method sets the Alexa Interface Message Sender to send Alexa Interface response events.
     *
     * @param alexaMessageSender The AlexaInterfaceMessageSenderInterface object.
     */
    virtual ExternalCapabilitiesBuilderInterface& withAlexaInterfaceMessageSender(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>
            alexaMessageSender) = 0;

    /**
     * This method sets the DialogUXStateAggregator for CallManager.
     *
     * @param dialogUXStateAggregator The DialogUXStateAggregator object.
     */
    virtual ExternalCapabilitiesBuilderInterface& withDialogUXStateAggregator(
        std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator) = 0;

    /**
     * Build the capabilities with the given core components.
     *
     * @param externalMediaPlayer Object used to manage external media playback.
     * @param connectionManager Object responsible for managing the SDK connection with AVS.
     * @param messageSender Object that can be used to send events to AVS.
     * @param exceptionSender Object that can be used to send exceptions to AVS.
     * @param certifiedSender Object that can be used to send events to AVS that require stronger guarantee.
     * @param audioFocusManager The focus manager for audio channels.
     * @param dataManager Object used to manage objects that store customer data.
     * @param stateReportHandler Object used to report the device state and its settings.
     * @param audioInputProcessor Object used to recognize voice interactions.
     * @param speakerManager Object used to manage all speaker instances that can be controlled by Alexa.
     * @param directiveSequencer Object used to sequence and handle a stream of @c AVSDirective instances.
     * @param userInactivityMonitor Object used to notify an implementation of the user activity.
     * @param contextManager Object used to provide the context for various components.
     * @param avsGatewayManager Object used to get the AVS gateway URL data.
     * @param ringtoneMediaPlayer The media player to play Comms ringtones.
     * @param audioFactory The audioFactory is a component the provides unique audio streams.
     * @param ringtoneChannelVolumeInterface The ChannelVolumeInterface used for ringtone channel volume attenuation.
#ifdef ENABLE_COMMS_AUDIO_PROXY
     * @param commsMediaPlayer The media player to play Comms calling audio.
     * @param commsSpeaker The speaker to control volume of Comms calling audio.
     * @param sharedDataStream The stream to use which has the audio from microphone.
#endif
     * @param powerResourceManager Object to manage power resource.
     * @param softwareComponentReporter Object to report adapters' versions.
     * @param playbackRouter Object to route local playback control command.
     * @param endpointRegistrationManager Object to manage endpoints.
     * @param metricRecorder Object to manage AVS SDK metric reporting.
     * @return A list with all capabilities as well as objects that require explicit shutdown. Shutdown will be
     * performed in the reverse order of occurrence.
     */
    virtual std::pair<std::list<Capability>, std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer> externalMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
        std::shared_ptr<capabilityAgents::system::ReportStateHandler> stateReportHandler,
        std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> audioInputProcessor,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface> avsGatewayManager,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> ringtoneChannelVolumeInterface,
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> commsMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker,
        std::shared_ptr<avsCommon::avs::AudioInputStream> sharedDataStream,
#endif
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ComponentReporterInterface> softwareComponentReporter,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
        std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationManagerInterface>
            endpointRegistrationManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) = 0;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_
