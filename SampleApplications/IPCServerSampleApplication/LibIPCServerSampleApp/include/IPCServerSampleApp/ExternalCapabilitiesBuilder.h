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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_

#include <AVSCommon/Utils/DeviceInfo.h>
#include <DefaultClient/ExternalCapabilitiesBuilderInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
/**
 * This class provides a facility to add external capabilities to default client.
 *
 * Default client will provide its core components in the build call.
 *
 * @note Any object created during the buildCapabilities that keep a pointer to a core component should be added or
 * managed by an object in the list of @c RequiresShutdown objects returned by buildCapabilities(). This will ensure
 * that these objects are shutdown during @c DefaultClient shutdown and before any core component is shutdown.
 */
class ExternalCapabilitiesBuilder : public defaultClient::ExternalCapabilitiesBuilderInterface {
public:
    /**
     * Constructor.
     *
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     */
    ExternalCapabilitiesBuilder(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// @name ExternalCapabilitiesBuilderInterface methods.
    /// @{
    ExternalCapabilitiesBuilder& withSettingsStorage(
        std::shared_ptr<settings::storage ::DeviceSettingStorageInterface> settingStorage) override;

    ExternalCapabilitiesBuilder& withTemplateRunTime(
        std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeInterface> templateRuntime) override;

    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> getCallManager() override;

    ExternalCapabilitiesBuilder& withInternetConnectionMonitor(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> internetConnectionMonitor)
        override;

    ExternalCapabilitiesBuilder& withDialogUXStateAggregator(
        std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator) override;
    ExternalCapabilitiesBuilderInterface& withAlexaInterfaceMessageSender(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>
            alexaMessageSender) override;
    std::pair<std::list<Capability>, std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>> buildCapabilities(
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
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) override;
    /// @}

private:
    /// DeviceInfo which reflects the device setup credentials.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// When GUI is enabled, this will hold the TemplateRuntime Capability Agent.
    std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeInterface> m_templateRuntime;

#ifdef ENABLE_COMMS
    /// When COMMS is enabled, this will hold the CallManager.
    std::shared_ptr<avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;
#endif

    /// When COMMS is enabled, this will hold the DialogUXStateAggregator
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_
